#pragma once

#include <QJsonObject>
#include <QString>

struct SupabaseSession {
    QString accessToken;
    QString refreshToken;
    QString userId;
    QString email;
    QString name;
};

class SupabaseAuth {
public:
    bool signIn(const QString &email, const QString &password, SupabaseSession &session, QString &error);
    bool signUp(const QString &email, const QString &password, const QString &name,
                SupabaseSession &session, QString &error);
    bool refresh(const QString &refreshToken, SupabaseSession &session, QString &error);
    bool sendPasswordReset(const QString &email, QString &error);

private:
    bool post(const QString &path, const QJsonObject &body, QJsonObject &reply, QString &error);
    static bool sessionFrom(const QJsonObject &reply, SupabaseSession &session);
};
