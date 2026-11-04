#pragma once

#include <QDate>
#include <QHash>
#include <QString>
#include <vector>

#include "cloudstore.h"

namespace Dashboard {

struct PersonRow {
    QString code;
    QString name;
    int workingDays = 0;
    int present = 0;
    int absent = 0;
    int late = 0;
    int unmarked = 0;
    double presenceRate = 0.0;
    double absenceRate = 0.0;
    double lateRate = 0.0;
};

struct Snapshot {
    QString title;
    int workingDays = 0;
    int present = 0;
    int absent = 0;
    int late = 0;
    int unmarked = 0;
    double presenceRate = 0.0;
    double absenceRate = 0.0;
    double lateRate = 0.0;
    std::vector<PersonRow> people;
};

Snapshot compute(const std::vector<CloudPerson> &people, int kind, const QHash<QString, CloudMarks> &marks,
                 int year, int month, int restMask, const QDate &today, const QString &title);

}
