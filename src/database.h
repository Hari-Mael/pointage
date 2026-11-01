#pragma once

#include <QChar>
#include <QDate>
#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <optional>
#include <vector>

struct UserRecord {
    int id = 0;
    QString email;
    QString name;
    QString passwordHash;
    int restMask = 1;
    int failedLogins = 0;
    qint64 lockedUntil = 0;
};

struct PersonRecord {
    int id = 0;
    QString code;
    QString name;
    QStringList fields;
};

using MarkMap = QMap<int, QChar>;

class Database {
public:
    bool open(const QString &path, QString &error);

    std::optional<UserRecord> findUser(const QString &email) const;
    bool createUser(const QString &email, const QString &name, const QString &passwordHash);
    bool recordFailedLogin(int userId, int failedLogins, qint64 lockedUntil);
    bool resetFailedLogins(int userId);
    bool setRestMask(int userId, int restMask);

    std::vector<PersonRecord> persons(int userId, int kind) const;
    QString nextCode(int userId, int kind, QChar prefix) const;
    bool addPerson(int userId, int kind, const PersonRecord &person);
    bool deletePerson(int personId);

    MarkMap marks(int personId, int year, int month) const;
    bool setMark(int personId, const QDate &date, QChar status);

private:
    QSqlDatabase connection() const;
};
