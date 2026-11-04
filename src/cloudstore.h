#pragma once

#include <QDate>
#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>
#include <vector>

#include "supabaseauth.h"

struct CloudPerson {
    QString id;
    int kind = 0;
    QString code;
    QString name;
    QStringList fields;
};

using CloudMarks = QMap<int, QChar>;

class CloudStore {
public:
    CloudStore(const SupabaseSession &session, const QString &dataDir);

    QString lastError() const { return m_lastError; }

    bool fetchPeople(std::vector<CloudPerson> &people);
    bool fetchMarks(int year, int month, QHash<QString, CloudMarks> &marks);
    bool fetchRestMask(int &mask);
    bool addPerson(const CloudPerson &person);
    bool deletePerson(const QString &personId);
    bool setMark(const QString &personId, const QDate &date, QChar status);
    bool clearMarks(const QString &personId);
    bool setRestMask(int mask);

private:
    bool send(const QByteArray &method, const QString &path, const QByteArray &body,
              const QByteArray &prefer, QByteArray &response);
    bool refreshSession();

    SupabaseSession m_session;
    QString m_dataDir;
    SupabaseAuth m_auth;
    QString m_lastError;
};
