#include "supabaseauth.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "config.h"

namespace {

QString messageFrom(const QJsonObject &reply)
{
    const char *keys[] = {"error_description", "msg", "message", "error"};
    for (const char *key : keys) {
        const QString text = reply.value(QLatin1String(key)).toString();
        if (!text.isEmpty())
            return text;
    }
    return {};
}

}

bool SupabaseAuth::post(const QString &path, const QJsonObject &body, QJsonObject &reply, QString &error)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(Config::kSupabaseUrl + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("apikey", Config::kSupabaseAnonKey.toUtf8());
    request.setRawHeader("Authorization", QByteArray("Bearer ") + Config::kSupabaseAnonKey.toUtf8());
    request.setTransferTimeout(15000);

    QNetworkReply *response = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QObject::connect(response, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool ok = response->error() == QNetworkReply::NoError;
    reply = QJsonDocument::fromJson(response->readAll()).object();
    error.clear();
    if (!ok) {
        const QString message = messageFrom(reply);
        error = message.isEmpty() ? QStringLiteral("Connexion au serveur impossible : ") + response->errorString()
                                  : message;
    }
    response->deleteLater();
    return ok;
}

bool SupabaseAuth::sessionFrom(const QJsonObject &reply, SupabaseSession &session)
{
    const QJsonObject user = reply.value(QLatin1String("user")).toObject();
    session.accessToken = reply.value(QLatin1String("access_token")).toString();
    session.refreshToken = reply.value(QLatin1String("refresh_token")).toString();
    session.userId = user.value(QLatin1String("id")).toString();
    session.email = user.value(QLatin1String("email")).toString();
    session.name = user.value(QLatin1String("user_metadata")).toObject().value(QLatin1String("name")).toString();
    return !session.accessToken.isEmpty();
}

bool SupabaseAuth::signIn(const QString &email, const QString &password, SupabaseSession &session, QString &error)
{
    QJsonObject reply;
    const QJsonObject body{{"email", email}, {"password", password}};
    if (!post(QStringLiteral("/auth/v1/token?grant_type=password"), body, reply, error))
        return false;
    return sessionFrom(reply, session);
}

bool SupabaseAuth::signUp(const QString &email, const QString &password, const QString &name,
                          SupabaseSession &session, QString &error)
{
    QJsonObject reply;
    const QJsonObject body{{"email", email},
                           {"password", password},
                           {"data", QJsonObject{{"name", name}}}};
    if (!post(QStringLiteral("/auth/v1/signup"), body, reply, error))
        return false;
    session = SupabaseSession();
    sessionFrom(reply, session);
    if (session.email.isEmpty())
        session.email = email;
    if (session.name.isEmpty())
        session.name = name;
    return true;
}

bool SupabaseAuth::refresh(const QString &refreshToken, SupabaseSession &session, QString &error)
{
    QJsonObject reply;
    const QJsonObject body{{"refresh_token", refreshToken}};
    if (!post(QStringLiteral("/auth/v1/token?grant_type=refresh_token"), body, reply, error))
        return false;
    return sessionFrom(reply, session);
}

bool SupabaseAuth::sendPasswordReset(const QString &email, QString &error)
{
    QJsonObject reply;
    const QJsonObject body{{"email", email}};
    return post(QStringLiteral("/auth/v1/recover"), body, reply, error);
}
