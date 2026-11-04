#include "logo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

namespace {

constexpr int kMaxLogoSize = 512;

QString customFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logo.png");
}

QPixmap currentSource()
{
    QPixmap source;
    if (QFileInfo::exists(customFile()))
        source.load(customFile());
    if (source.isNull())
        source.load(QStringLiteral(":/logo.png"));
    return source;
}

}

namespace Logo {

QPixmap pixmap(int size)
{
    return currentSource().scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QIcon icon()
{
    return QIcon(currentSource().scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

bool setCustom(const QString &sourcePath, QString &error)
{
    QImage image(sourcePath);
    if (image.isNull()) {
        error = QStringLiteral("image illisible");
        return false;
    }
    if (image.width() > kMaxLogoSize || image.height() > kMaxLogoSize)
        image = image.scaled(kMaxLogoSize, kMaxLogoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QDir().mkpath(QFileInfo(customFile()).absolutePath());
    if (!image.save(customFile(), "PNG")) {
        error = QStringLiteral("enregistrement impossible");
        return false;
    }
    error.clear();
    return true;
}

void resetCustom()
{
    QFile::remove(customFile());
}

}
