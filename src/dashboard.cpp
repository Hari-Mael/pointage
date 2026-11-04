#include "dashboard.h"

#include "reports.h"

namespace Dashboard {

Snapshot compute(const std::vector<CloudPerson> &people, int kind, const QHash<QString, CloudMarks> &marks,
                 int year, int month, int restMask, const QDate &today, const QString &title)
{
    Snapshot snapshot;
    snapshot.title = title;

    for (const CloudPerson &person : people) {
        if (person.kind != kind)
            continue;

        const Reports::Stats stats = Reports::computeStats(marks.value(person.id), year, month, restMask, today);
        PersonRow row;
        row.code = person.code;
        row.name = person.name;
        row.workingDays = stats.workingDays;
        row.present = stats.present;
        row.absent = stats.absent;
        row.late = stats.late;
        row.unmarked = stats.unmarked;
        row.presenceRate = stats.presenceRate();
        row.absenceRate = stats.absenceRate();
        row.lateRate = stats.lateRate();
        snapshot.people.push_back(row);

        snapshot.workingDays += stats.workingDays;
        snapshot.present += stats.present;
        snapshot.absent += stats.absent;
        snapshot.late += stats.late;
        snapshot.unmarked += stats.unmarked;
    }

    if (snapshot.workingDays > 0)
        snapshot.presenceRate = static_cast<double>(snapshot.present + snapshot.late) / snapshot.workingDays;
    if (snapshot.workingDays > 0)
        snapshot.absenceRate = static_cast<double>(snapshot.absent) / snapshot.workingDays;
    if (snapshot.present + snapshot.late > 0)
        snapshot.lateRate = static_cast<double>(snapshot.late) / (snapshot.present + snapshot.late);

    return snapshot;
}

}
