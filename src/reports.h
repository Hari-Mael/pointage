#pragma once

#include <QDate>
#include <QString>
#include <QStringList>
#include <vector>

#include "cloudstore.h"

namespace Reports {

enum class Rate { Presence, Absence, Late };

struct Stats {
    int workingDays = 0;
    int present = 0;
    int absent = 0;
    int late = 0;
    int unmarked = 0;

    double presenceRate() const;
    double absenceRate() const;
    double lateRate() const;
};

struct RosterImport {
    std::vector<CloudPerson> people;
    QStringList errors;
    int skippedExisting = 0;
};

Stats computeStats(const CloudMarks &marks, int year, int month, int restMask, const QDate &today);
QString monthlyDetailCsv(const CloudPerson &person, const CloudMarks &marks, int year, int month,
                         int restMask, const QDate &today);
QString rateCsv(const CloudPerson &person, const CloudMarks &marks, int year, int month, int restMask,
                const QDate &today, Rate rate);
QString rosterCsv(const std::vector<CloudPerson> &people, int kind);
RosterImport rosterFromCsv(const QString &content, int kind, const QStringList &existingCodes);
QString decode(const QByteArray &bytes);

}
