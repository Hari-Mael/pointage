#include "studentreport.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLocale>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

#include "reports.h"

namespace {

QString percentText(double value)
{
    return QString::number(value * 100.0, 'f', 1).replace(QLatin1Char('.'), QLatin1Char(',')) + QStringLiteral(" %");
}

QString badgeColor(double presenceRate)
{
    if (presenceRate >= 0.9)
        return QStringLiteral("#1b8a4b");
    if (presenceRate >= 0.75)
        return QStringLiteral("#b7791f");
    return QStringLiteral("#d12f3a");
}

QString statusText(QChar mark, bool rest, bool future)
{
    if (rest)
        return QStringLiteral("Repos");
    if (future)
        return QStringLiteral("À venir");
    if (mark == QChar('P'))
        return QStringLiteral("Présent");
    if (mark == QChar('A'))
        return QStringLiteral("Absent");
    if (mark == QChar('R'))
        return QStringLiteral("Retard");
    return QStringLiteral("Non pointé");
}

}

namespace StudentReport {

QString toHtml(const CloudPerson &person, const CloudMarks &marks, int year, int month, int restMask,
               const QDate &today, const QString &schoolName)
{
    const QLocale french(QLocale::French);
    const Reports::Stats stats = Reports::computeStats(marks, year, month, restMask, today);
    const int days = QDate(year, month, 1).daysInMonth();
    const QString monthLabel = french.monthName(month) + QLatin1Char(' ') + QString::number(year);
    const QString color = badgeColor(stats.presenceRate());

    QString rows;
    for (int day = 1; day <= days; ++day) {
        const QDate date(year, month, day);
        const bool rest = (restMask >> (date.dayOfWeek() % 7)) & 1;
        const QString status = statusText(marks.value(day), rest, date > today);
        const QString background = (day % 2 == 0) ? QStringLiteral("#f5f7fa") : QStringLiteral("#ffffff");
        rows += QStringLiteral("<tr style=\"background:%1\"><td>%2</td><td>%3</td><td>%4</td></tr>")
                    .arg(background, date.toString(QStringLiteral("dd/MM/yyyy")),
                         french.dayName(date.dayOfWeek(), QLocale::LongFormat), status.toHtmlEscaped());
    }

    return QStringLiteral(
               "<html><head><style>"
               "body { font-family: 'Segoe UI', Arial, sans-serif; color: #1d2733; }"
               "h1 { font-size: 18pt; margin: 0; } h2 { font-size: 12pt; color: #6b7785; margin: 2px 0 14px; }"
               "table { border-collapse: collapse; width: 100%; }"
               "th { background: #0b3b5c; color: #ffffff; padding: 6px; text-align: left; }"
               "td { padding: 5px 6px; border-bottom: 1px solid #e8ecf2; }"
               ".badge { display: inline-block; padding: 8px 14px; border-radius: 12px; color: #ffffff; "
               "font-size: 14pt; font-weight: bold; }"
               ".summary td { border: none; padding: 3px 6px; }"
               ".sign { margin-top: 40px; color: #6b7785; }"
               "</style></head><body>"
               "<h1>%1</h1>"
               "<h2>Rapport de présence — %2</h2>"
               "<table class=\"summary\" width=\"100%\"><tr>"
               "<td><b>Élève</b> : %3</td><td><b>Classe</b> : %4</td><td><b>Matricule</b> : %5</td>"
               "</tr></table>"
               "<p><span class=\"badge\" style=\"background:%6\">Présentéisme %7</span></p>"
               "<table class=\"summary\" width=\"100%\"><tr>"
               "<td>Jours travaillés : <b>%8</b></td><td>Présents : <b>%9</b></td>"
               "<td>Retards : <b>%10</b></td><td>Absents : <b>%11</b></td><td>Non pointés : <b>%12</b></td>"
               "</tr><tr>"
               "<td>Taux d'absentéisme : <b>%13</b></td><td>Taux de retard : <b>%14</b></td>"
               "</tr></table>"
               "<table width=\"100%\"><tr><th>Date</th><th>Jour</th><th>Statut</th></tr>%15</table>"
               "<p class=\"sign\">Édité le %16 · Signature de la direction : ____________________</p>"
               "</body></html>")
        .arg(schoolName.toHtmlEscaped(), monthLabel.toHtmlEscaped(), person.name.toHtmlEscaped(),
             person.className.isEmpty() ? QStringLiteral("—") : person.className.toHtmlEscaped(),
             person.code.toHtmlEscaped(), color, percentText(stats.presenceRate()))
        .arg(stats.workingDays)
        .arg(stats.present)
        .arg(stats.late)
        .arg(stats.absent)
        .arg(stats.unmarked)
        .arg(percentText(stats.absenceRate()), percentText(stats.lateRate()), rows,
             french.toString(QDate::currentDate(), QStringLiteral("dd MMMM yyyy")));
}

}

StudentReportDialog::StudentReportDialog(const CloudPerson &person, const CloudMarks &marks, int year, int month,
                                         int restMask, const QDate &today, const QString &schoolName, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Rapport élève — %1").arg(person.name));
    resize(760, 820);

    auto *layout = new QVBoxLayout(this);
    m_view = new QTextBrowser(this);
    m_view->setHtml(StudentReport::toHtml(person, marks, year, month, restMask, today, schoolName));
    layout->addWidget(m_view, 1);

    auto *buttons = new QHBoxLayout;
    auto *printButton = new QPushButton(tr("Imprimer"), this);
    printButton->setProperty("variant", QStringLiteral("teal"));
    auto *pdfButton = new QPushButton(tr("Télécharger en PDF"), this);
    pdfButton->setProperty("variant", QStringLiteral("ghost"));
    auto *closeButton = new QPushButton(tr("Fermer"), this);
    closeButton->setProperty("variant", QStringLiteral("ghost"));
    buttons->addStretch(1);
    buttons->addWidget(printButton);
    buttons->addWidget(pdfButton);
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);

    connect(printButton, &QPushButton::clicked, this, &StudentReportDialog::print);
    connect(pdfButton, &QPushButton::clicked, this, &StudentReportDialog::saveAsPdf);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void StudentReportDialog::print()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_view->document()->print(&printer);
}

void StudentReportDialog::saveAsPdf()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Enregistrer le rapport en PDF"),
                                                      QStringLiteral("rapport_eleve.pdf"),
                                                      tr("Document PDF (*.pdf)"));
    if (path.isEmpty())
        return;
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    m_view->document()->print(&printer);
    QMessageBox::information(this, windowTitle(), tr("Rapport enregistré : %1").arg(path));
}
