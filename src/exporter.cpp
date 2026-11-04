#include "exporter.h"

#include <QDate>
#include <QSaveFile>
#include <QStringList>

namespace {

QString csvField(const QString &value)
{
    if (value.contains(QLatin1Char(';')) || value.contains(QLatin1Char('"')) || value.contains(QLatin1Char('\n'))) {
        QString escaped = value;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return value;
}

}

namespace Exporter {

QString buildCsv(const std::vector<CloudPerson> &people, const QHash<QString, CloudMarks> &marks,
                 int kind, int year, int month)
{
    const int days = QDate(year, month, 1).daysInMonth();

    QStringList header{QStringLiteral("ID"), QStringLiteral("Nom")};
    for (int day = 1; day <= days; ++day)
        header << QString::number(day);
    header << QStringLiteral("Présences") << QStringLiteral("Absences");

    QStringList lines{header.join(QLatin1Char(';'))};
    for (const CloudPerson &person : people) {
        if (person.kind != kind)
            continue;

        const CloudMarks personMarks = marks.value(person.id);
        QStringList row{csvField(person.code), csvField(person.name)};
        int present = 0;
        int absent = 0;
        for (int day = 1; day <= days; ++day) {
            const QChar status = personMarks.value(day);
            if (status == QChar('P'))
                ++present;
            else if (status == QChar('A'))
                ++absent;
            row << (status.isNull() ? QString() : QString(status));
        }
        row << QString::number(present) << QString::number(absent);
        lines << row.join(QLatin1Char(';'));
    }

    return QString(QChar(0xFEFF)) + lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

bool writeCsv(const QString &path, const QString &content, QString &error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        error = file.errorString();
        return false;
    }
    error.clear();
    return true;
}

}
