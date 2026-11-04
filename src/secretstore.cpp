#include "secretstore.h"

#include <QDir>
#include <QFile>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {

QString fileFor(const QString &dataDir)
{
    return dataDir + QStringLiteral("/session.bin");
}

}

namespace SecretStore {

#ifdef _WIN32

bool save(const QString &dataDir, const QString &secret)
{
    QByteArray plain = secret.toUtf8();
    DATA_BLOB in{};
    in.cbData = static_cast<DWORD>(plain.size());
    in.pbData = reinterpret_cast<BYTE *>(plain.data());
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"Pointage session", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        return false;

    QDir().mkpath(dataDir);
    QFile file(fileFor(dataDir));
    const bool ok = file.open(QIODevice::WriteOnly)
                    && file.write(reinterpret_cast<const char *>(out.pbData), static_cast<qint64>(out.cbData))
                           == static_cast<qint64>(out.cbData);
    LocalFree(out.pbData);
    return ok;
}

QString load(const QString &dataDir)
{
    QFile file(fileFor(dataDir));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QByteArray cipher = file.readAll();
    DATA_BLOB in{};
    in.cbData = static_cast<DWORD>(cipher.size());
    in.pbData = reinterpret_cast<BYTE *>(cipher.data());
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        return {};
    const QString text = QString::fromUtf8(reinterpret_cast<const char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return text;
}

#else

bool save(const QString &, const QString &)
{
    return false;
}

QString load(const QString &)
{
    return {};
}

#endif

void clear(const QString &dataDir)
{
    QFile::remove(fileFor(dataDir));
}

}
