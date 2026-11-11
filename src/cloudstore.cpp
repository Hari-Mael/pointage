#include "cloudstore.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "config.h"
#include "secretstore.h"

namespace {

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

}

CloudStore::CloudStore(const SupabaseSession &session, const QString &dataDir)
    : m_session(session), m_dataDir(dataDir)
{
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

bool CloudStore::send(const QByteArray &method, const QString &path, const QByteArray &body,
                      const QByteArray &prefer, QByteArray &response)
{
    for (int attempt = 0; attempt < 2; ++attempt) {
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl(Config::kSupabaseUrl + path));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setRawHeader("apikey", Config::kSupabaseAnonKey.toUtf8());
        request.setRawHeader("Authorization", QByteArray("Bearer ") + m_session.accessToken.toUtf8());
        if (!prefer.isEmpty())
            request.setRawHeader("Prefer", prefer);
        request.setTransferTimeout(15000);

        QNetworkReply *reply = manager.sendCustomRequest(request, method, body);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
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

bool CloudStore::fetchPeople(std::vector<CloudPerson> &people)
{
    QByteArray response;
    if (!send("GET", QStringLiteral("/rest/v1/persons?select=id,kind,code,name,fields&order=code"), {}, {}, response))
        return false;

    people.clear();
    for (const QJsonValue &value : QJsonDocument::fromJson(response).array()) {
        const QJsonObject row = value.toObject();
        CloudPerson person;
        person.id = row.value(QLatin1String("id")).toString();
        person.kind = row.value(QLatin1String("kind")).toInt();
        person.code = row.value(QLatin1String("code")).toString();
        person.name = row.value(QLatin1String("name")).toString();
        for (const QJsonValue &field : row.value(QLatin1String("fields")).toArray())
            person.fields << field.toString();
        people.push_back(person);
    }
    return true;
}

bool CloudStore::fetchMarks(int year, int month, QHash<QString, CloudMarks> &marks)
{
    const QDate first(year, month, 1);
    const QDate last(year, month, first.daysInMonth());
    const QString path = QStringLiteral("/rest/v1/marks?select=person_id,day,status&day=gte.%1&day=lte.%2")
                             .arg(first.toString(Qt::ISODate), last.toString(Qt::ISODate));
    QByteArray response;
    if (!send("GET", path, {}, {}, response))
        return false;

    marks.clear();
    for (const QJsonValue &value : QJsonDocument::fromJson(response).array()) {
        const QJsonObject row = value.toObject();
        const QDate day = QDate::fromString(row.value(QLatin1String("day")).toString(), Qt::ISODate);
        const QString status = row.value(QLatin1String("status")).toString();
        if (!day.isValid() || status.isEmpty())
            continue;
        marks[row.value(QLatin1String("person_id")).toString()].insert(day.day(), status.at(0));
    }
    return true;
}

bool CloudStore::fetchRestMask(int &mask)
{
    const QString path = QStringLiteral("/rest/v1/user_settings?select=rest_mask&owner_id=eq.%1").arg(m_session.userId);
    QByteArray response;
    if (!send("GET", path, {}, {}, response))
        return false;

    const QJsonArray rows = QJsonDocument::fromJson(response).array();
    mask = rows.isEmpty() ? 1 : rows.first().toObject().value(QLatin1String("rest_mask")).toInt(1);
    return true;
}

bool CloudStore::addPerson(const CloudPerson &person)
{
    QJsonArray fields;
    for (const QString &field : person.fields)
        fields.append(field);
    const QJsonObject body{{"owner_id", m_session.userId},
                           {"kind", person.kind},
                           {"code", person.code},
                           {"name", person.name},
                           {"fields", fields}};
    QByteArray response;
    return send("POST", QStringLiteral("/rest/v1/persons"), QJsonDocument(body).toJson(QJsonDocument::Compact),
                "return=minimal", response);
}

bool CloudStore::updatePerson(const CloudPerson &person)
{
    QJsonArray fields;
    for (const QString &field : person.fields)
        fields.append(field);
    const QJsonObject body{{"code", person.code}, {"name", person.name}, {"fields", fields}};
    QByteArray response;
    return send("PATCH", QStringLiteral("/rest/v1/persons?id=eq.%1").arg(person.id),
                QJsonDocument(body).toJson(QJsonDocument::Compact), "return=minimal", response);
}

bool CloudStore::deletePerson(const QString &personId)
{
    QByteArray response;
    return send("DELETE", QStringLiteral("/rest/v1/persons?id=eq.%1").arg(personId), {}, "return=minimal", response);
}

bool CloudStore::setMark(const QString &personId, const QDate &date, QChar status)
{
    QByteArray response;
    const QString day = date.toString(Qt::ISODate);
    if (status.isNull())
        return send("DELETE", QStringLiteral("/rest/v1/marks?person_id=eq.%1&day=eq.%2").arg(personId, day), {},
                    "return=minimal", response);

    const QJsonObject body{{"person_id", personId}, {"day", day}, {"status", QString(status)}};
    return send("POST", QStringLiteral("/rest/v1/marks?on_conflict=person_id,day"),
                QJsonDocument(body).toJson(QJsonDocument::Compact),
                "resolution=merge-duplicates,return=minimal", response);
}

bool CloudStore::clearMarks(const QString &personId)
{
    QByteArray response;
    return send("DELETE", QStringLiteral("/rest/v1/marks?person_id=eq.%1").arg(personId), {}, "return=minimal",
                response);
}

bool CloudStore::setRestMask(int mask)
{
    const QJsonObject body{{"owner_id", m_session.userId}, {"rest_mask", mask}};
    QByteArray response;
    return send("POST", QStringLiteral("/rest/v1/user_settings?on_conflict=owner_id"),
                QJsonDocument(body).toJson(QJsonDocument::Compact),
                "resolution=merge-duplicates,return=minimal", response);
}
