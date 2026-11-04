#include <QTest>

#include <cmath>

#include "fields.h"
#include "reports.h"

class TstReports : public QObject {
    Q_OBJECT

private slots:
    void rateValuesNovember2026();
    void stopsAtToday();
    void rosterRoundTrip();
    void rosterRejectsBadRows();
    void rosterSkipsExistingCodes();
};

namespace {

CloudMarks marksNovember()
{
    CloudMarks marks;
    marks.insert(2, QChar('P'));
    marks.insert(3, QChar('P'));
    marks.insert(4, QChar('A'));
    marks.insert(5, QChar('R'));
    return marks;
}

bool near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

}

void TstReports::rateValuesNovember2026()
{
    const Reports::Stats stats = Reports::computeStats(marksNovember(), 2026, 11, 1, QDate(2026, 11, 30));
    QCOMPARE(stats.workingDays, 25);
    QCOMPARE(stats.present, 2);
    QCOMPARE(stats.absent, 1);
    QCOMPARE(stats.late, 1);
    QCOMPARE(stats.unmarked, 21);
    QVERIFY(near(stats.presenceRate(), 3.0 / 25.0));
    QVERIFY(near(stats.absenceRate(), 1.0 / 25.0));
    QVERIFY(near(stats.lateRate(), 1.0 / 3.0));
}

void TstReports::stopsAtToday()
{
    const Reports::Stats stats = Reports::computeStats(marksNovember(), 2026, 11, 1, QDate(2026, 11, 10));
    QCOMPARE(stats.workingDays, 8);
}

void TstReports::rosterRoundTrip()
{
    CloudPerson person;
    person.kind = kKindEmployee;
    person.code = QStringLiteral("E010");
    person.name = QStringLiteral("Jean Dupont");
    person.fields = QStringList(18);
    person.fields[0] = QStringLiteral("Jean");
    person.fields[1] = QStringLiteral("Dupont");
    person.fields[4] = QStringLiteral("+261 34 12 345 67");
    person.fields[5] = QStringLiteral("jean@exemple.fr");

    const QString csv = Reports::rosterCsv({person}, kKindEmployee);
    const Reports::RosterImport result = Reports::rosterFromCsv(csv, kKindEmployee, {});
    QVERIFY(result.errors.isEmpty());
    QCOMPARE(result.people.size(), size_t(1));
    QCOMPARE(result.people.front().code, QStringLiteral("E010"));
    QCOMPARE(result.people.front().name, QStringLiteral("Jean Dupont"));
    QCOMPARE(result.people.front().fields.at(5), QStringLiteral("jean@exemple.fr"));
}

void TstReports::rosterRejectsBadRows()
{
    const QString csv = QStringLiteral("Code;Prénom;Nom;Email\n"
                                       "E020;Marie;Rakoto;pas-un-email\n"
                                       ";Paul;Martin;\n"
                                       "E021;;Ravelo;\n");
    const Reports::RosterImport result = Reports::rosterFromCsv(csv, kKindEmployee, {});
    QVERIFY(result.people.empty());
    QCOMPARE(result.errors.size(), 3);
}

void TstReports::rosterSkipsExistingCodes()
{
    const QString csv = QStringLiteral("Code;Prénom;Nom\nE030;Anna;Razafy\nE031;Hery;Rabe\n");
    const Reports::RosterImport result = Reports::rosterFromCsv(csv, kKindEmployee, {QStringLiteral("E030")});
    QCOMPARE(result.skippedExisting, 1);
    QCOMPARE(result.people.size(), size_t(1));
    QCOMPARE(result.people.front().code, QStringLiteral("E031"));
}

QTEST_MAIN(TstReports)
#include "tst_reports.moc"
