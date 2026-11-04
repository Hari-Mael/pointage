#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>

#include "dashboard.h"
#include "excelreport.h"
#include "fields.h"

class TstDashboard : public QObject {
    Q_OBJECT

private slots:
    void computeRatesForGroup();
    void writesExcelReportFile();
};

namespace {

CloudPerson makePerson(const QString &id, const QString &code)
{
    CloudPerson person;
    person.id = id;
    person.kind = kKindEmployee;
    person.code = code;
    person.name = QStringLiteral("Nom %1").arg(code);
    return person;
}

bool near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

}

void TstDashboard::computeRatesForGroup()
{
    const std::vector<CloudPerson> people{makePerson(QStringLiteral("p1"), QStringLiteral("E001")),
                                          makePerson(QStringLiteral("p2"), QStringLiteral("E002"))};
    QHash<QString, CloudMarks> marks;
    marks[QStringLiteral("p1")].insert(2, QChar('P'));
    marks[QStringLiteral("p1")].insert(3, QChar('R'));
    marks[QStringLiteral("p1")].insert(4, QChar('A'));
    marks[QStringLiteral("p2")].insert(2, QChar('P'));

    const Dashboard::Snapshot snapshot = Dashboard::compute(people, kKindEmployee, marks, 2026, 11, 1,
                                                            QDate(2026, 11, 10), QStringLiteral("Test"));
    QCOMPARE(snapshot.people.size(), size_t(2));
    QCOMPARE(snapshot.workingDays, 16);
    QCOMPARE(snapshot.present, 2);
    QCOMPARE(snapshot.late, 1);
    QCOMPARE(snapshot.absent, 1);
    QVERIFY(near(snapshot.presenceRate, 3.0 / 16.0));
    QVERIFY(near(snapshot.absenceRate, 1.0 / 16.0));
    QVERIFY(near(snapshot.lateRate, 1.0 / 3.0));
}

void TstDashboard::writesExcelReportFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("rapport.xlsx"));

    Dashboard::Snapshot snapshot;
    snapshot.title = QStringLiteral("Test");
    snapshot.workingDays = 10;
    snapshot.present = 8;
    snapshot.absent = 1;
    snapshot.late = 1;
    Dashboard::PersonRow row;
    row.code = QStringLiteral("E001");
    row.name = QStringLiteral("Nom");
    row.workingDays = 10;
    row.present = 8;
    row.late = 1;
    row.absent = 1;
    snapshot.people.push_back(row);

    QString error;
    QVERIFY2(ExcelReport::writeMonthlyReport(path, snapshot, error), qPrintable(error));
    QVERIFY(QFileInfo(path).size() > 1000);
}

QTEST_MAIN(TstDashboard)
#include "tst_dashboard.moc"
