#pragma once

#include <QIcon>
#include <QPixmap>
#include <QString>

namespace Logo {

QPixmap pixmap(int size);
QIcon icon();
bool setCustom(const QString &sourcePath, QString &error);
void resetCustom();

}
