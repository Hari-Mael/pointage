#include "auth.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QStringList>

namespace {

constexpr int kIterations = 600000;
constexpr int kSaltLength = 16;

QByteArray deriveKey(const QByteArray &password, const QByteArray &salt, int iterations)
{
    const QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
    QByteArray u = QMessageAuthenticationCode::hash(salt + QByteArray("\0\0\0\1", 4), password, algorithm);
    QByteArray t = u;
    for (int i = 1; i < iterations; ++i) {
        u = QMessageAuthenticationCode::hash(u, password, algorithm);
        for (int j = 0; j < t.size(); ++j)
            t[j] = static_cast<char>(t[j] ^ u[j]);
    }
    return t;
}

bool constantTimeEqual(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size())
        return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}

}

namespace Auth {

QString hashPassword(const QString &password)
{
    QByteArray salt(kSaltLength, Qt::Uninitialized);
    QRandomGenerator *rng = QRandomGenerator::system();
    for (int i = 0; i < kSaltLength; ++i)
        salt[i] = static_cast<char>(rng->generate() & 0xFF);

    const QByteArray key = deriveKey(password.toUtf8(), salt, kIterations);
    return QStringLiteral("pbkdf2_sha256$%1$%2$%3")
        .arg(kIterations)
        .arg(QString::fromLatin1(salt.toHex()))
        .arg(QString::fromLatin1(key.toHex()));
}

bool verifyPassword(const QString &password, const QString &stored)
{
    const QStringList parts = stored.split(QLatin1Char('$'));
    if (parts.size() != 4 || parts[0] != QLatin1String("pbkdf2_sha256"))
        return false;

    bool ok = false;
    const int iterations = parts[1].toInt(&ok);
    if (!ok || iterations < 1)
        return false;

    const QByteArray salt = QByteArray::fromHex(parts[2].toLatin1());
    const QByteArray expected = QByteArray::fromHex(parts[3].toLatin1());
    const QByteArray actual = deriveKey(password.toUtf8(), salt, iterations);
    return constantTimeEqual(actual, expected);
}

bool selfTest()
{
    const QByteArray out = deriveKey(QByteArray("password"), QByteArray("salt"), 2);
    return out.toHex() == QByteArray("ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
}

}
