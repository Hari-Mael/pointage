#include "database.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

const QString kConnectionName = QStringLiteral("pointage");

QString fieldsToJson(const QStringList &fields)
{
    QJsonArray array;
    for (const QString &value : fields)
        array.append(value);
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QStringList fieldsFromJson(const QString &json)
{
    QStringList fields;
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &value : array)
        fields << value.toString();
    return fields;
}

bool runQuery(QSqlQuery &query, const QString &sql, const QVariantList &args)
{
    if (!query.prepare(sql))
        return false;
    for (const QVariant &arg : args)
        query.addBindValue(arg);
    return query.exec();
}

}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(kConnectionName);
}

bool Database::open(const QString &path, QString &error)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        error = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    const QStringList statements = {
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS users ("
                       "id INTEGER PRIMARY KEY, "
                       "email TEXT UNIQUE NOT NULL, "
                       "name TEXT NOT NULL, "
                       "password_hash TEXT NOT NULL, "
                       "rest_mask INTEGER NOT NULL DEFAULT 1, "
                       "failed_logins INTEGER NOT NULL DEFAULT 0, "
                       "locked_until INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS persons ("
                       "id INTEGER PRIMARY KEY, "
                       "owner_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE, "
                       "kind INTEGER NOT NULL, "
                       "code TEXT NOT NULL, "
                       "name TEXT NOT NULL, "
                       "fields TEXT NOT NULL, "
                       "UNIQUE (owner_id, kind, code))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS marks ("
                       "person_id INTEGER NOT NULL REFERENCES persons(id) ON DELETE CASCADE, "
                       "day TEXT NOT NULL, "
                       "status TEXT NOT NULL CHECK (status IN ('P', 'A')), "
                       "PRIMARY KEY (person_id, day))"),
    };
    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            error = query.lastError().text();
            return false;
        }
    }
    return true;
}

std::optional<UserRecord> Database::findUser(const QString &email) const
{
    QSqlQuery query(connection());
    if (!runQuery(query,
                  QStringLiteral("SELECT id, email, name, password_hash, rest_mask, failed_logins, locked_until "
                                 "FROM users WHERE email = ?"),
                  {email})
        || !query.next())
        return std::nullopt;

    UserRecord user;
    user.id = query.value(0).toInt();
    user.email = query.value(1).toString();
    user.name = query.value(2).toString();
    user.passwordHash = query.value(3).toString();
    user.restMask = query.value(4).toInt();
    user.failedLogins = query.value(5).toInt();
    user.lockedUntil = query.value(6).toLongLong();
    return user;
}

bool Database::createUser(const QString &email, const QString &name, const QString &passwordHash)
{
    QSqlQuery query(connection());
    return runQuery(query,
                    QStringLiteral("INSERT INTO users (email, name, password_hash) VALUES (?, ?, ?)"),
                    {email, name, passwordHash});
}

bool Database::recordFailedLogin(int userId, int failedLogins, qint64 lockedUntil)
{
    QSqlQuery query(connection());
    return runQuery(query,
                    QStringLiteral("UPDATE users SET failed_logins = ?, locked_until = ? WHERE id = ?"),
                    {failedLogins, lockedUntil, userId});
}

bool Database::resetFailedLogins(int userId)
{
    QSqlQuery query(connection());
    return runQuery(query,
                    QStringLiteral("UPDATE users SET failed_logins = 0, locked_until = 0 WHERE id = ?"),
                    {userId});
}

bool Database::setRestMask(int userId, int restMask)
{
    QSqlQuery query(connection());
    return runQuery(query, QStringLiteral("UPDATE users SET rest_mask = ? WHERE id = ?"), {restMask, userId});
}

std::vector<PersonRecord> Database::persons(int userId, int kind) const
{
    std::vector<PersonRecord> people;
    QSqlQuery query(connection());
    if (!runQuery(query,
                  QStringLiteral("SELECT id, code, name, fields FROM persons "
                                 "WHERE owner_id = ? AND kind = ? ORDER BY code"),
                  {userId, kind}))
        return people;

    while (query.next()) {
        PersonRecord person;
        person.id = query.value(0).toInt();
        person.code = query.value(1).toString();
        person.name = query.value(2).toString();
        person.fields = fieldsFromJson(query.value(3).toString());
        people.push_back(person);
    }
    return people;
}

QString Database::nextCode(int userId, int kind, QChar prefix) const
{
    int highest = 0;
    QSqlQuery query(connection());
    if (runQuery(query,
                 QStringLiteral("SELECT code FROM persons WHERE owner_id = ? AND kind = ?"),
                 {userId, kind})) {
        while (query.next()) {
            const QString code = query.value(0).toString();
            if (!code.startsWith(prefix))
                continue;
            bool ok = false;
            const int number = code.mid(1).toInt(&ok);
            if (ok && number > highest)
                highest = number;
        }
    }
    return QStringLiteral("%1%2").arg(prefix).arg(highest + 1, 3, 10, QLatin1Char('0'));
}

bool Database::addPerson(int userId, int kind, const PersonRecord &person)
{
    QSqlQuery query(connection());
    return runQuery(query,
                    QStringLiteral("INSERT INTO persons (owner_id, kind, code, name, fields) VALUES (?, ?, ?, ?, ?)"),
                    {userId, kind, person.code, person.name, fieldsToJson(person.fields)});
}

bool Database::deletePerson(int personId)
{
    QSqlQuery query(connection());
    return runQuery(query, QStringLiteral("DELETE FROM persons WHERE id = ?"), {personId});
}

bool Database::clearMarks(int personId)
{
    QSqlQuery query(connection());
    return runQuery(query, QStringLiteral("DELETE FROM marks WHERE person_id = ?"), {personId});
}

MarkMap Database::marks(int personId, int year, int month) const
{
    MarkMap result;
    const QString monthPattern = QStringLiteral("%1-%2-%")
                                     .arg(year)
                                     .arg(month, 2, 10, QLatin1Char('0'));
    QSqlQuery query(connection());
    if (!runQuery(query,
                  QStringLiteral("SELECT day, status FROM marks WHERE person_id = ? AND day LIKE ?"),
                  {personId, monthPattern}))
        return result;

    while (query.next()) {
        const QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        if (date.isValid())
            result.insert(date.day(), query.value(1).toString().at(0));
    }
    return result;
}

bool Database::setMark(int personId, const QDate &date, QChar status)
{
    QSqlQuery query(connection());
    const QString day = date.toString(Qt::ISODate);
    if (status.isNull())
        return runQuery(query, QStringLiteral("DELETE FROM marks WHERE person_id = ? AND day = ?"), {personId, day});
    return runQuery(query,
                    QStringLiteral("INSERT INTO marks (person_id, day, status) VALUES (?, ?, ?) "
                                   "ON CONFLICT(person_id, day) DO UPDATE SET status = excluded.status"),
                    {personId, day, QString(status)});
}
