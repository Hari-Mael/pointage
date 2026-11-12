#pragma once

#include <QChar>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

struct LocalOp {
    qint64 seq = 0;
    QString kind;
    QJsonObject payload;
};

class LocalDb {
public:
    LocalDb() = default;
    ~LocalDb();
    LocalDb(const LocalDb &) = delete;
    LocalDb &operator=(const LocalDb &) = delete;

    bool open(const QString &directory, const QString &userId, QString &error);
    void close();

    bool upsertPerson(const QJsonObject &person);
    bool deletePerson(const QString &personId);
    bool setMark(const QString &personId, const QDate &date, QChar status);
    bool clearMarks(const QString &personId);
    bool setRestMask(int mask);
    bool setAvatar(const QString &avatar);

    bool storeRemotePerson(const QJsonObject &person);
    bool purgePersonsExcept(const QStringList &ids);
    bool replaceMarksOfMonth(int year, int month, const QJsonArray &rows);
    bool storeRemoteSettings(int restMask, const QString &avatar);

    QJsonArray persons() const;
    QJsonArray marksForMonth(int year, int month) const;
    int restMask() const;
    QString avatar() const;

    QList<LocalOp> pendingOps(int limit = 100) const;
    bool markSynced(qint64 seq);
    int pendingCount() const;

private:
    bool enqueue(const QString &kind, const QJsonObject &payload);
    bool exec(const QString &sql, const QVariantList &values = {}) const;
    bool writePerson(const QJsonObject &person) const;

    QString m_connection;
};
