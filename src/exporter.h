#pragma once

#include <QHash>
#include <QString>
#include <vector>

#include "cloudstore.h"

namespace Exporter {

QString buildCsv(const std::vector<CloudPerson> &people, const QHash<QString, CloudMarks> &marks,
                 int kind, int year, int month);
bool writeCsv(const QString &path, const QString &content, QString &error);

}
