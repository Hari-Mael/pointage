#pragma once

#include <QString>

namespace SecretStore {

bool save(const QString &dataDir, const QString &secret);
QString load(const QString &dataDir);
void clear(const QString &dataDir);

}
