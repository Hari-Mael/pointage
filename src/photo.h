#pragma once

#include <QPixmap>
#include <QString>

namespace Photo {

QString encodeFile(const QString &path, QString &error);
QPixmap decode(const QString &encoded);
QPixmap avatar(const QString &encoded, const QString &name, int size);
QString initials(const QString &name);

}
