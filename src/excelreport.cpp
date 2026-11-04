#include "excelreport.h"

#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxworksheet.h"

using namespace QXlsx;

namespace {

constexpr int kPercentFormat = 10;

Format percentFormat()
{
    Format format;
    format.setNumberFormatIndex(kPercentFormat);
    return format;
}

void writeSummary(Document &doc, const Dashboard::Snapshot &snapshot)
{
    doc.write(QStringLiteral("A1"), snapshot.title);
    doc.renameSheet(QStringLiteral("Sheet1"), QStringLiteral("Synthèse"));
    Worksheet *sheet = doc.currentWorksheet();

    doc.write(QStringLiteral("A3"), QStringLiteral("Indicateur"));
    doc.write(QStringLiteral("B3"), QStringLiteral("Valeur"));
    doc.write(QStringLiteral("A4"), QStringLiteral("Taux de présentéisme"));
    doc.write(QStringLiteral("B4"), snapshot.presenceRate, percentFormat());
    doc.write(QStringLiteral("A5"), QStringLiteral("Taux d'absentéisme"));
    doc.write(QStringLiteral("B5"), snapshot.absenceRate, percentFormat());
    doc.write(QStringLiteral("A6"), QStringLiteral("Taux de retard"));
    doc.write(QStringLiteral("B6"), snapshot.lateRate, percentFormat());
    doc.write(QStringLiteral("A7"), QStringLiteral("Jours travaillés"));
    doc.write(QStringLiteral("B7"), snapshot.workingDays);

    doc.write(QStringLiteral("A9"), QStringLiteral("Statut"));
    doc.write(QStringLiteral("B9"), QStringLiteral("Jours"));
    doc.write(QStringLiteral("A10"), QStringLiteral("Présents"));
    doc.write(QStringLiteral("B10"), snapshot.present);
    doc.write(QStringLiteral("A11"), QStringLiteral("Retards"));
    doc.write(QStringLiteral("B11"), snapshot.late);
    doc.write(QStringLiteral("A12"), QStringLiteral("Absents"));
    doc.write(QStringLiteral("B12"), snapshot.absent);
    doc.write(QStringLiteral("A13"), QStringLiteral("Non pointés"));
    doc.write(QStringLiteral("B13"), snapshot.unmarked);

    Chart *pie = sheet->insertChart(2, 3, QSize(460, 320));
    pie->setChartType(Chart::CT_PieChart);
    pie->setChartTitle(QStringLiteral("Répartition des jours"));
    pie->addSeries(CellRange(QStringLiteral("A10:B13")), nullptr, false, true);
}

void writeDetail(Document &doc, const Dashboard::Snapshot &snapshot)
{
    doc.addSheet(QStringLiteral("Détail"));
    doc.selectSheet(QStringLiteral("Détail"));
    Worksheet *sheet = doc.currentWorksheet();

    const QStringList headers{QStringLiteral("Code"), QStringLiteral("Nom"), QStringLiteral("Jours travaillés"),
                              QStringLiteral("Présents"), QStringLiteral("Retards"), QStringLiteral("Absents"),
                              QStringLiteral("Taux de présentéisme"), QStringLiteral("Taux d'absentéisme"),
                              QStringLiteral("Taux de retard")};
    for (int column = 0; column < headers.size(); ++column)
        doc.write(1, column + 1, headers.at(column));

    int row = 2;
    for (const Dashboard::PersonRow &person : snapshot.people) {
        doc.write(row, 1, person.code);
        doc.write(row, 2, person.name);
        doc.write(row, 3, person.workingDays);
        doc.write(row, 4, person.present);
        doc.write(row, 5, person.late);
        doc.write(row, 6, person.absent);
        doc.write(row, 7, person.presenceRate, percentFormat());
        doc.write(row, 8, person.absenceRate, percentFormat());
        doc.write(row, 9, person.lateRate, percentFormat());
        ++row;
    }

    if (snapshot.people.empty())
        return;

    const int last = row - 1;
    Chart *bars = sheet->insertChart(row + 1, 1, QSize(720, 360));
    bars->setChartType(Chart::CT_BarChart);
    bars->setChartTitle(QStringLiteral("Taux par personne"));
    bars->addSeries(CellRange(QStringLiteral("G2:G%1").arg(last)), nullptr, false, false);
    bars->addSeries(CellRange(QStringLiteral("H2:H%1").arg(last)), nullptr, false, false);
    bars->addSeries(CellRange(QStringLiteral("I2:I%1").arg(last)), nullptr, false, false);
}

}

namespace ExcelReport {

bool writeMonthlyReport(const QString &path, const Dashboard::Snapshot &snapshot, QString &error)
{
    Document doc;
    writeSummary(doc, snapshot);
    writeDetail(doc, snapshot);
    if (!doc.saveAs(path)) {
        error = QStringLiteral("enregistrement impossible");
        return false;
    }
    error.clear();
    return true;
}

}
