#pragma once

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>
#include <vector>

#include "localdb.h"
#include "supabaseauth.h"

struct CloudPerson {
    QString id;
    int kind = 0;
    QString code;
    QString name;
    QString className;
    QString photo;
    QStringList fields;
};

using CloudMarks = QMap<int, QChar>;

class CloudStore {
public:
    CloudStore(const SupabaseSession &session, const QString &dataDir);

    QString lastError() const { return m_lastError; }
    int pendingChanges() const { return m_local.pendingCount(); }

    bool fetchPeople(std::vector<CloudPerson> &people);
    bool fetchMarks(int year, int month, QHash<QString, CloudMarks> &marks);
    bool fetchRestMask(int &mask);
    bool addPerson(const CloudPerson &person);
    bool updatePerson(const CloudPerson &person);
    bool deletePerson(const QString &personId);
    bool setMark(const QString &personId, const QDate &date, QChar status);
    bool clearMarks(const QString &personId);
    bool setRestMask(int mask);
    bool fetchAvatar(QString &avatar);
    bool setAvatar(const QString &avatar);

private:
    bool request(const QByteArray &method, const QString &path, const QByteArray &body,
                 const QByteArray &prefer, QByteArray &response, int &status);
    bool refreshSession();
    bool buildOperation(const LocalOp &op, QByteArray &method, QString &path, QByteArray &body, QByteArray &prefer) const;
    bool flushOutbox();
    void synchronize(int year, int month);
    bool pullAll(int year, int month);
    void flushNow();

    SupabaseSession m_session;
    QString m_dataDir;
    SupabaseAuth m_auth;
    QString m_lastError;
    LocalDb m_local;
    QDateTime m_lastPull;
    int m_pullYear = 0;
    int m_pullMonth = 0;
};
