#pragma once

#include <QString>

#include "dashboard.h"

namespace ExcelReport {

bool writeMonthlyReport(const QString &path, const Dashboard::Snapshot &snapshot, QString &error);

}
