#include "cloudstore.h"

#include <QDir>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

#include "config.h"
#include "secretstore.h"

namespace {

constexpr int kPullIntervalSeconds = 20;
constexpr int kMaxFlushRounds = 100;

QString messageFromBody(const QByteArray &body, const QString &fallback)
{
    const QJsonObject object = QJsonDocument::fromJson(body).object();
    const char *keys[] = {"message", "msg", "error_description", "hint", "details"};
    for (const char *key : keys) {
        const QString text = object.value(QLatin1String(key)).toString();
        if (!text.isEmpty())
            return text;
    }
    return fallback;
}

QJsonObject personToJson(const CloudPerson &person)
{
    QJsonArray fields;
    for (const QString &field : person.fields)
        fields.append(field);
    return QJsonObject{{QStringLiteral("id"), person.id},
                       {QStringLiteral("kind"), person.kind},
                       {QStringLiteral("code"), person.code},
                       {QStringLiteral("name"), person.name},
                       {QStringLiteral("class_name"), person.className},
                       {QStringLiteral("photo"), person.photo},
                       {QStringLiteral("fields"), fields}};
}

CloudPerson personFromJson(const QJsonObject &row)
{
    CloudPerson person;
    person.id = row.value(QLatin1String("id")).toString();
    person.kind = row.value(QLatin1String("kind")).toInt();
    person.code = row.value(QLatin1String("code")).toString();
    person.name = row.value(QLatin1String("name")).toString();
    person.className = row.value(QLatin1String("class_name")).toString();
    person.photo = row.value(QLatin1String("photo")).toString();
    for (const QJsonValue &field : row.value(QLatin1String("fields")).toArray())
        person.fields << field.toString();
    return person;
}

}

CloudStore::CloudStore(const SupabaseSession &session, const QString &dataDir)
    : m_session(session), m_dataDir(dataDir)
{
    const QDate today = QDate::currentDate();
    m_pullYear = today.year();
    m_pullMonth = today.month();

    QString error;
    if (!m_local.open(QDir(dataDir).filePath(QStringLiteral("local")), session.userId, error))
        m_lastError = QStringLiteral("Base locale indisponible : %1").arg(error);
}

bool CloudStore::refreshSession()
{
    SupabaseSession refreshed;
    QString error;
    if (!m_auth.refresh(m_session.refreshToken, refreshed, error)) {
        m_lastError = QStringLiteral("Session expirée, reconnectez-vous.");
        return false;
    }
    m_session.accessToken = refreshed.accessToken;
    m_session.refreshToken = refreshed.refreshToken;
    if (!SecretStore::load(m_dataDir).isEmpty())
        SecretStore::save(m_dataDir, refreshed.refreshToken);
    return true;
}

bool CloudStore::request(const QByteArray &method, const QString &path, const QByteArray &body,
                         const QByteArray &prefer, QByteArray &response, int &status)
{
    for (int attempt = 0; attempt < 2; ++attempt) {
        QNetworkAccessManager manager;
        QNetworkRequest networkRequest(QUrl(Config::kSupabaseUrl + path));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        networkRequest.setRawHeader("apikey", Config::kSupabaseAnonKey.toUtf8());
        networkRequest.setRawHeader("Authorization", QByteArray("Bearer ") + m_session.accessToken.toUtf8());
        if (!prefer.isEmpty())
            networkRequest.setRawHeader("Prefer", prefer);
        networkRequest.setTransferTimeout(15000);

        QNetworkReply *reply = manager.sendCustomRequest(networkRequest, method, body);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString networkError = reply->errorString();
        response = reply->readAll();
        reply->deleteLater();

        if (status == 401 && attempt == 0 && refreshSession())
            continue;
        if (!ok) {
            m_lastError = messageFromBody(response, QStringLiteral("Serveur injoignable : ") + networkError);
            return false;
        }
        return true;
    }
    return false;
}

bool CloudStore::buildOperation(const LocalOp &op, QByteArray &method, QString &path, QByteArray &body,
                                QByteArray &prefer) const
{
    const QJsonObject &payload = op.payload;
    const QByteArray merge = "resolution=merge-duplicates,return=minimal";
    const QByteArray minimal = "return=minimal";

    if (op.kind == QLatin1String("person.upsert")) {
        QJsonObject row = payload;
        row.insert(QStringLiteral("owner_id"), m_session.userId);
        method = "POST";
        path = QStringLiteral("/rest/v1/persons?on_conflict=id");
        body = QJsonDocument(row).toJson(QJsonDocument::Compact);
        prefer = merge;
        return true;
    }
    if (op.kind == QLatin1String("person.delete")) {
        method = "DELETE";
        path = QStringLiteral("/rest/v1/persons?id=eq.%1").arg(payload.value(QStringLiteral("id")).toString());
        prefer = minimal;
        return true;
    }
    if (op.kind == QLatin1String("mark.set")) {
        const QString personId = payload.value(QStringLiteral("person_id")).toString();
        const QString day = payload.value(QStringLiteral("day")).toString();
        if (payload.value(QStringLiteral("status")).isNull()) {
            method = "DELETE";
            path = QStringLiteral("/rest/v1/marks?person_id=eq.%1&day=eq.%2").arg(personId, day);
            prefer = minimal;
        } else {
            method = "POST";
            path = QStringLiteral("/rest/v1/marks?on_conflict=person_id,day");
            body = QJsonDocument(QJsonObject{{QStringLiteral("person_id"), personId},
                                             {QStringLiteral("day"), day},
                                             {QStringLiteral("status"), payload.value(QStringLiteral("status")).toString()}})
                       .toJson(QJsonDocument::Compact);
            prefer = merge;
        }
        return true;
    }
    if (op.kind == QLatin1String("marks.clear")) {
        method = "DELETE";
        path = QStringLiteral("/rest/v1/marks?person_id=eq.%1").arg(payload.value(QStringLiteral("person_id")).toString());
        prefer = minimal;
        return true;
    }
    if (op.kind == QLatin1String("settings.rest") || op.kind == QLatin1String("settings.avatar")) {
        QJsonObject row{{QStringLiteral("owner_id"), m_session.userId}};
        for (auto it = payload.begin(); it != payload.end(); ++it)
            row.insert(it.key(), it.value());
        method = "POST";
        path = QStringLiteral("/rest/v1/user_settings?on_conflict=owner_id");
        body = QJsonDocument(row).toJson(QJsonDocument::Compact);
        prefer = merge;
        return true;
    }
    return false;
}

bool CloudStore::flushOutbox()
{
    if (m_session.accessToken.isEmpty())
        return false;

    for (int round = 0; round < kMaxFlushRounds; ++round) {
        const QList<LocalOp> ops = m_local.pendingOps(50);
        if (ops.isEmpty())
            return true;

        for (const LocalOp &op : ops) {
            QByteArray method;
            QString path;
            QByteArray body;
            QByteArray prefer;
            if (!buildOperation(op, method, path, body, prefer)) {
                m_local.markSynced(op.seq);
                continue;
            }

            QByteArray response;
            int status = 0;
            if (!request(method, path, body, prefer, response, status)) {
                if (status == 0)
                    return false;
                m_local.markSynced(op.seq);
                continue;
            }
            m_local.markSynced(op.seq);
        }
    }
    return false;
}

bool CloudStore::pullAll(int year, int month)
{
    QByteArray response;
    int status = 0;

    if (!request("GET", QStringLiteral("/rest/v1/persons?select=id,kind,code,name,class_name,photo,fields&order=code"),
                 {}, {}, response, status))
        return false;
    const QJsonArray persons = QJsonDocument::fromJson(response).array();
    QStringList ids;
    for (const QJsonValue &value : persons) {
        const QJsonObject row = value.toObject();
        ids << row.value(QStringLiteral("id")).toString();
        m_local.storeRemotePerson(row);
    }
    m_local.purgePersonsExcept(ids);

    const QDate first(year, month, 1);
    const QDate last(year, month, first.daysInMonth());
    const QString marksPath = QStringLiteral("/rest/v1/marks?select=person_id,day,status&day=gte.%1&day=lte.%2")
                                  .arg(first.toString(Qt::ISODate), last.toString(Qt::ISODate));
    if (!request("GET", marksPath, {}, {}, response, status))
        return false;
    m_local.replaceMarksOfMonth(year, month, QJsonDocument::fromJson(response).array());

    if (!request("GET", QStringLiteral("/rest/v1/user_settings?select=rest_mask,avatar&owner_id=eq.%1").arg(m_session.userId),
                 {}, {}, response, status))
        return false;
    const QJsonArray settings = QJsonDocument::fromJson(response).array();
    if (!settings.isEmpty()) {
        const QJsonObject row = settings.first().toObject();
        m_local.storeRemoteSettings(row.value(QStringLiteral("rest_mask")).toInt(1),
                                    row.value(QStringLiteral("avatar")).toString());
    }

    m_lastPull = QDateTime::currentDateTime();
    return true;
}

void CloudStore::synchronize(int year, int month)
{
    m_pullYear = year;
    m_pullMonth = month;
    if (m_session.accessToken.isEmpty())
        return;

    flushOutbox();
    if (m_local.pendingCount() > 0)
        return;

    const bool fresh = m_lastPull.isValid() && m_lastPull.secsTo(QDateTime::currentDateTime()) < kPullIntervalSeconds;
    if (!fresh)
        pullAll(year, month);
}

void CloudStore::flushNow()
{
    flushOutbox();
}

bool CloudStore::fetchPeople(std::vector<CloudPerson> &people)
{
    synchronize(m_pullYear, m_pullMonth);
    people.clear();
    for (const QJsonValue &value : m_local.persons())
        people.push_back(personFromJson(value.toObject()));
    return true;
}

bool CloudStore::fetchMarks(int year, int month, QHash<QString, CloudMarks> &marks)
{
    synchronize(year, month);
    marks.clear();
    for (const QJsonValue &value : m_local.marksForMonth(year, month)) {
        const QJsonObject row = value.toObject();
        const QString status = row.value(QStringLiteral("status")).toString();
        if (status.isEmpty())
            continue;
        marks[row.value(QStringLiteral("person_id")).toString()].insert(
            row.value(QStringLiteral("day")).toInt(), status.at(0));
    }
    return true;
}

bool CloudStore::fetchRestMask(int &mask)
{
    synchronize(m_pullYear, m_pullMonth);
    mask = m_local.restMask();
    return true;
}

bool CloudStore::fetchAvatar(QString &avatar)
{
    synchronize(m_pullYear, m_pullMonth);
    avatar = m_local.avatar();
    return true;
}

bool CloudStore::addPerson(const CloudPerson &person)
{
    CloudPerson created = person;
    if (created.id.isEmpty())
        created.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!m_local.upsertPerson(personToJson(created))) {
        m_lastError = QStringLiteral("Enregistrement local impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}

bool CloudStore::updatePerson(const CloudPerson &person)
{
    if (!m_local.upsertPerson(personToJson(person))) {
        m_lastError = QStringLiteral("Enregistrement local impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}

bool CloudStore::deletePerson(const QString &personId)
{
    if (!m_local.deletePerson(personId)) {
        m_lastError = QStringLiteral("Suppression locale impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}

bool CloudStore::setMark(const QString &personId, const QDate &date, QChar status)
{
    if (!m_local.setMark(personId, date, status)) {
        m_lastError = QStringLiteral("Enregistrement local impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}

bool CloudStore::clearMarks(const QString &personId)
{
    if (!m_local.clearMarks(personId)) {
        m_lastError = QStringLiteral("Effacement local impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}

bool CloudStore::setRestMask(int mask)
{
    if (!m_local.setRestMask(mask)) {
        m_lastError = QStringLiteral("Enregistrement local impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}

bool CloudStore::setAvatar(const QString &avatar)
{
    if (!m_local.setAvatar(avatar)) {
        m_lastError = QStringLiteral("Enregistrement local impossible.");
        return false;
    }
    m_lastError.clear();
    flushNow();
    return true;
}
