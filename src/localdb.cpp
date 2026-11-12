#include "localdb.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

QString isoNow()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

}

LocalDb::~LocalDb()
{
    close();
}

bool LocalDb::open(const QString &directory, const QString &userId, QString &error)
{
    close();
    QDir().mkpath(directory);
    m_connection = QStringLiteral("pointage-%1").arg(userId);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(QDir(directory).filePath(QStringLiteral("%1.db").arg(userId)));
    if (!db.open()) {
        error = db.lastError().text();
        QSqlDatabase::removeDatabase(m_connection);
        m_connection.clear();
        return false;
    }

    const QStringList schema{
        QStringLiteral("CREATE TABLE IF NOT EXISTS persons (id TEXT PRIMARY KEY, kind INTEGER NOT NULL, code TEXT NOT NULL, "
                       "name TEXT NOT NULL, class_name TEXT NOT NULL DEFAULT '', photo TEXT NOT NULL DEFAULT '', "
                       "fields TEXT NOT NULL DEFAULT '[]', updated_at TEXT NOT NULL, deleted INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS marks (person_id TEXT NOT NULL, day TEXT NOT NULL, status TEXT NOT NULL, "
                       "updated_at TEXT NOT NULL, PRIMARY KEY (person_id, day))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS settings (id INTEGER PRIMARY KEY CHECK (id = 1), "
                       "rest_mask INTEGER NOT NULL DEFAULT 1, avatar TEXT NOT NULL DEFAULT '')"),
        QStringLiteral("INSERT OR IGNORE INTO settings (id, rest_mask, avatar) VALUES (1, 1, '')"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS outbox (seq INTEGER PRIMARY KEY AUTOINCREMENT, kind TEXT NOT NULL, "
                       "payload TEXT NOT NULL, created_at TEXT NOT NULL)"),
    };
    for (const QString &sql : schema) {
        if (!exec(sql)) {
            error = QSqlDatabase::database(m_connection).lastError().text();
            close();
            return false;
        }
    }
    error.clear();
    return true;
}

void LocalDb::close()
{
    if (m_connection.isEmpty())
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connection, false);
        if (db.isValid())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connection);
    m_connection.clear();
}

bool LocalDb::exec(const QString &sql, const QVariantList &values) const
{
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(sql);
    for (const QVariant &value : values)
        query.addBindValue(value);
    return query.exec();
}

bool LocalDb::enqueue(const QString &kind, const QJsonObject &payload)
{
    return exec(QStringLiteral("INSERT INTO outbox (kind, payload, created_at) VALUES (?, ?, ?)"),
                {kind, QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)), isoNow()});
}

bool LocalDb::upsertPerson(const QJsonObject &person)
{
    const QString id = person.value(QStringLiteral("id")).toString();
    const QString fields = QString::fromUtf8(QJsonDocument(person.value(QStringLiteral("fields")).toArray())
                                                 .toJson(QJsonDocument::Compact));
    const bool ok = exec(QStringLiteral(
                             "INSERT INTO persons (id, kind, code, name, class_name, photo, fields, updated_at, deleted) "
                             "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0) ON CONFLICT(id) DO UPDATE SET kind=excluded.kind, "
                             "code=excluded.code, name=excluded.name, class_name=excluded.class_name, photo=excluded.photo, "
                             "fields=excluded.fields, updated_at=excluded.updated_at, deleted=0"),
                         {id, person.value(QStringLiteral("kind")).toInt(), person.value(QStringLiteral("code")).toString(),
                          person.value(QStringLiteral("name")).toString(),
                          person.value(QStringLiteral("class_name")).toString(),
                          person.value(QStringLiteral("photo")).toString(), fields, isoNow()});
    return ok && enqueue(QStringLiteral("person.upsert"), person);
}

bool LocalDb::deletePerson(const QString &personId)
{
    const bool ok = exec(QStringLiteral("UPDATE persons SET deleted = 1, updated_at = ? WHERE id = ?"), {isoNow(), personId})
                    && exec(QStringLiteral("DELETE FROM marks WHERE person_id = ?"), {personId});
    return ok && enqueue(QStringLiteral("person.delete"), QJsonObject{{QStringLiteral("id"), personId}});
}

bool LocalDb::setMark(const QString &personId, const QDate &date, QChar status)
{
    const QString day = date.toString(Qt::ISODate);
    bool ok = true;
    if (status.isNull())
        ok = exec(QStringLiteral("DELETE FROM marks WHERE person_id = ? AND day = ?"), {personId, day});
    else
        ok = exec(QStringLiteral("INSERT INTO marks (person_id, day, status, updated_at) VALUES (?, ?, ?, ?) "
                                 "ON CONFLICT(person_id, day) DO UPDATE SET status=excluded.status, updated_at=excluded.updated_at"),
                  {personId, day, QString(status), isoNow()});
    return ok && enqueue(QStringLiteral("mark.set"),
                         QJsonObject{{QStringLiteral("person_id"), personId},
                                     {QStringLiteral("day"), day},
                                     {QStringLiteral("status"), status.isNull() ? QJsonValue(QJsonValue::Null)
                                                                                : QJsonValue(QString(status))}});
}

bool LocalDb::clearMarks(const QString &personId)
{
    return exec(QStringLiteral("DELETE FROM marks WHERE person_id = ?"), {personId})
           && enqueue(QStringLiteral("marks.clear"), QJsonObject{{QStringLiteral("person_id"), personId}});
}

bool LocalDb::setRestMask(int mask)
{
    return exec(QStringLiteral("UPDATE settings SET rest_mask = ? WHERE id = 1"), {mask})
           && enqueue(QStringLiteral("settings.rest"), QJsonObject{{QStringLiteral("rest_mask"), mask}});
}

bool LocalDb::setAvatar(const QString &avatar)
{
    return exec(QStringLiteral("UPDATE settings SET avatar = ? WHERE id = 1"), {avatar})
           && enqueue(QStringLiteral("settings.avatar"), QJsonObject{{QStringLiteral("avatar"), avatar}});
}

QJsonArray LocalDb::persons() const
{
    QJsonArray result;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.exec(QStringLiteral("SELECT id, kind, code, name, class_name, photo, fields FROM persons WHERE deleted = 0 ORDER BY code"));
    while (query.next()) {
        QJsonObject person{{QStringLiteral("id"), query.value(0).toString()},
                           {QStringLiteral("kind"), query.value(1).toInt()},
                           {QStringLiteral("code"), query.value(2).toString()},
                           {QStringLiteral("name"), query.value(3).toString()},
                           {QStringLiteral("class_name"), query.value(4).toString()},
                           {QStringLiteral("photo"), query.value(5).toString()},
                           {QStringLiteral("fields"), QJsonDocument::fromJson(query.value(6).toString().toUtf8()).array()}};
        result.append(person);
    }
    return result;
}

QJsonObject LocalDb::marksOf(const QString &personId) const
{
    QJsonObject result;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(QStringLiteral("SELECT day, status FROM marks WHERE person_id = ?"));
    query.addBindValue(personId);
    query.exec();
    while (query.next()) {
        const QDate day = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        result.insert(QString::number(day.day()), query.value(1).toString());
    }
    return result;
}

int LocalDb::restMask() const
{
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.exec(QStringLiteral("SELECT rest_mask FROM settings WHERE id = 1"));
    return query.next() ? query.value(0).toInt() : 1;
}

QString LocalDb::avatar() const
{
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.exec(QStringLiteral("SELECT avatar FROM settings WHERE id = 1"));
    return query.next() ? query.value(0).toString() : QString();
}

QList<LocalOp> LocalDb::pendingOps(int limit) const
{
    QList<LocalOp> result;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(QStringLiteral("SELECT seq, kind, payload FROM outbox ORDER BY seq LIMIT ?"));
    query.addBindValue(limit);
    query.exec();
    while (query.next()) {
        LocalOp op;
        op.seq = query.value(0).toLongLong();
        op.kind = query.value(1).toString();
        op.payload = QJsonDocument::fromJson(query.value(2).toString().toUtf8()).object();
        result.append(op);
    }
    return result;
}

bool LocalDb::markSynced(qint64 seq)
{
    return exec(QStringLiteral("DELETE FROM outbox WHERE seq = ?"), {seq});
}

int LocalDb::pendingCount() const
{
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.exec(QStringLiteral("SELECT COUNT(*) FROM outbox"));
    return query.next() ? query.value(0).toInt() : 0;
}
