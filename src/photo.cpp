#include "photo.h"

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>

namespace {

constexpr int kStoredSize = 160;

}

namespace Photo {

QString encodeFile(const QString &path, QString &error)
{
    QImage image(path);
    if (image.isNull()) {
        error = QStringLiteral("image illisible");
        return {};
    }
    const int side = qMin(image.width(), image.height());
    image = image.copy((image.width() - side) / 2, (image.height() - side) / 2, side, side)
                .scaled(kStoredSize, kStoredSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "JPEG", 80)) {
        error = QStringLiteral("encodage impossible");
        return {};
    }
    error.clear();
    return QString::fromLatin1(bytes.toBase64());
}

QPixmap decode(const QString &encoded)
{
    QPixmap pixmap;
    pixmap.loadFromData(QByteArray::fromBase64(encoded.toLatin1()), "JPEG");
    return pixmap;
}

QString initials(const QString &name)
{
    const QStringList words = name.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QString result;
    for (int i = 0; i < words.size() && i < 2; ++i)
        result += words.at(i).left(1).toUpper();
    return result;
}

QPixmap avatar(const QString &encoded, const QString &name, int size)
{
    QPixmap out(size, size);
    out.fill(Qt::transparent);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRect rect(0, 0, size, size);
    QPainterPath clip;
    clip.addEllipse(rect);
    painter.setClipPath(clip);

    if (!encoded.isEmpty()) {
        painter.drawPixmap(rect, decode(encoded).scaled(size, size, Qt::KeepAspectRatioByExpanding,
                                                        Qt::SmoothTransformation));
    } else {
        const int hue = static_cast<int>(qHash(name) % 360);
        painter.fillRect(rect, QColor::fromHsv(hue, 90, 200));
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(size * 40 / 100);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(rect, Qt::AlignCenter, initials(name));
    }
    return out;
}

}
