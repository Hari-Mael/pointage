#pragma once

#include <QString>

namespace Auth {

QString hashPassword(const QString &password);
bool verifyPassword(const QString &password, const QString &stored);
bool selfTest();

}
