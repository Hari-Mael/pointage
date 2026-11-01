#include "presenceprinter.h"

#include <QDate>
#include <QDialog>
#include <QLocale>
#include <QPageLayout>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextDocument>

namespace {

bool isRestDay(const PresenceSheet &sheet, int day)
{
    const int weekday = QDate(sheet.year, sheet.month, day).dayOfWeek();
    return (sheet.restMask >> (weekday % 7)) & 1;
}

}

QString presenceHtml(const PresenceSheet &sheet)
{
    const QLocale french(QLocale::French);
    const int days = QDate(sheet.year, sheet.month, 1).daysInMonth();

    QString html = QStringLiteral(
        "<html><head><meta charset=\"utf-8\"><style>"
        "body{font-family:sans-serif;font-size:8pt}"
        "table{border-collapse:collapse;width:100%}"
        "th,td{border:1px solid #888;padding:2px;text-align:center}"
        "th{background-color:#0b3b5c;color:#ffffff}"
        "td.name{text-align:left;white-space:nowrap}"
        "</style></head><body>");

    html += QStringLiteral("<h2>%1</h2><p>%2 %3</p>")
                .arg(sheet.title.toHtmlEscaped())
                .arg(french.monthName(sheet.month))
                .arg(sheet.year);

    html += QStringLiteral("<table><tr><th>ID</th><th>Nom</th>");
    for (int day = 1; day <= days; ++day) {
        const int weekday = QDate(sheet.year, sheet.month, day).dayOfWeek();
        html += QStringLiteral("<th>%1<br>%2</th>")
                    .arg(french.dayName(weekday, QLocale::ShortFormat).left(2))
                    .arg(day);
    }
    html += QStringLiteral("<th>P</th><th>A</th></tr>");

    for (const PresenceRow &row : sheet.rows) {
        int present = 0;
        int absent = 0;
        html += QStringLiteral("<tr><td>%1</td><td class=\"name\">%2</td>")
                    .arg(row.code.toHtmlEscaped())
                    .arg(row.name.toHtmlEscaped());

        for (int day = 1; day <= days; ++day) {
            if (isRestDay(sheet, day)) {
                html += QStringLiteral("<td style=\"background-color:#e6eaf0\"></td>");
                continue;
            }
            const QChar status = row.marks.value(day);
            if (status == QChar('P')) {
                ++present;
                html += QStringLiteral("<td style=\"background-color:#dff5e6;color:#1b8a4b;font-weight:bold\">P</td>");
            } else if (status == QChar('A')) {
                ++absent;
                html += QStringLiteral("<td style=\"background-color:#fde2e4;color:#d12f3a;font-weight:bold\">A</td>");
            } else {
                html += QStringLiteral("<td></td>");
            }
        }
        html += QStringLiteral("<td>%1</td><td>%2</td></tr>").arg(present).arg(absent);
    }

    html += QStringLiteral("</table></body></html>");
    return html;
}

void printPresenceSheet(QWidget *parent, const PresenceSheet &sheet)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Landscape);

    QPrintDialog dialog(&printer, parent);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QTextDocument document;
    document.setHtml(presenceHtml(sheet));
    document.print(&printer);
}
