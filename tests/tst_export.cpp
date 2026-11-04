#include <QTemporaryDir>
#include <QTest>

#include "exporter.h"

class TstExport : public QObject {
    Q_OBJECT

private slots:
    void csvHasBomHeaderAndRows();
    void csvFiltersByKind();
    void csvEscapesSemicolonAndQuotes();
    void csvCountsPresencesAndAbsences();
    void writeCsvCreatesFile();
};

namespace {

CloudPerson makePerson(const QString &id, int kind, const QString &code, const QString &name)
{
    CloudPerson person;
    person.id = id;
    person.kind = kind;
    person.code = code;
    person.name = name;
    return person;
}

}

void TstExport::csvHasBomHeaderAndRows()
{
    const std::vector<CloudPerson> people{makePerson(QStringLiteral("p1"), 0, QStringLiteral("E001"), QStringLiteral("Jean Dupont"))};
    const QString csv = Exporter::buildCsv(people, {}, 0, 2026, 11);

    QCOMPARE(csv.at(0), QChar(0xFEFF));
    const QStringList lines = csv.mid(1).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QCOMPARE(lines.size(), 2);
    QVERIFY(lines.first().startsWith(QStringLiteral("ID;Nom;1;2;3")));
    QVERIFY(lines.first().endsWith(QStringLiteral(";30;Présences;Absences")));
    QVERIFY(lines.at(1).startsWith(QStringLiteral("E001;Jean Dupont;")));
}

void TstExport::csvFiltersByKind()
{
    const std::vector<CloudPerson> people{
        makePerson(QStringLiteral("p1"), 0, QStringLiteral("E001"), QStringLiteral("Jean")),
        makePerson(QStringLiteral("p2"), 1, QStringLiteral("M001"), QStringLiteral("Marie")),
    };
    const QString students = Exporter::buildCsv(people, {}, 1, 2026, 11);
    QVERIFY(students.contains(QStringLiteral("M001")));
    QVERIFY(!students.contains(QStringLiteral("E001")));
}

void TstExport::csvEscapesSemicolonAndQuotes()
{
    const std::vector<CloudPerson> people{
        makePerson(QStringLiteral("p1"), 0, QStringLiteral("E002"), QStringLiteral("Dupont; \"Jean\"")),
    };
    const QString csv = Exporter::buildCsv(people, {}, 0, 2026, 11);
    QVERIFY(csv.contains(QStringLiteral("E002;\"Dupont; \"\"Jean\"\"\";")));
}

void TstExport::csvCountsPresencesAndAbsences()
{
    const std::vector<CloudPerson> people{makePerson(QStringLiteral("p1"), 0, QStringLiteral("E003"), QStringLiteral("Test"))};
    QHash<QString, CloudMarks> marks;
    marks[QStringLiteral("p1")].insert(2, QChar('P'));
    marks[QStringLiteral("p1")].insert(3, QChar('A'));
    marks[QStringLiteral("p1")].insert(4, QChar('A'));

    const QString csv = Exporter::buildCsv(people, marks, 0, 2026, 11);
    const QStringList lines = csv.mid(1).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QStringList cells = lines.at(1).split(QLatin1Char(';'));
    QCOMPARE(cells.at(3), QStringLiteral("P"));
    QCOMPARE(cells.at(4), QStringLiteral("A"));
    QCOMPARE(cells.at(cells.size() - 2), QStringLiteral("1"));
    QCOMPARE(cells.at(cells.size() - 1), QStringLiteral("2"));
}

void TstExport::writeCsvCreatesFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("export.csv"));
    QString error;
    QVERIFY2(Exporter::writeCsv(path, QStringLiteral("ID;Nom\n"), error), qPrintable(error));
    QVERIFY(QFileInfo::exists(path));
}

QTEST_MAIN(TstExport)
#include "tst_export.moc"
