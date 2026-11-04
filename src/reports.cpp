#include "reports.h"

#include <QLocale>
#include <QStringDecoder>

#include "fields.h"

namespace {

const QChar kSep(';');

QString bom()
{
    return QString(QChar(0xFEFF));
}

QString csvField(const QString &value)
{
    if (value.contains(kSep) || value.contains(QLatin1Char('"')) || value.contains(QLatin1Char('\n'))) {
        QString escaped = value;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return value;
}

QString csvLine(const QStringList &fields)
{
    QStringList escaped;
    for (const QString &field : fields)
        escaped << csvField(field);
    return escaped.join(kSep);
}

QString percent(double value)
{
    return QString::number(value * 100.0, 'f', 1).replace(QLatin1Char('.'), QLatin1Char(',')) + QStringLiteral(" %");
}

bool isRest(int restMask, const QDate &date)
{
    return (restMask >> (date.dayOfWeek() % 7)) & 1;
}

std::vector<QStringList> parseCsv(const QString &content)
{
    QString text = content;
    if (!text.isEmpty() && text.at(0) == QChar(0xFEFF))
        text.remove(0, 1);

    const QChar delimiter = text.section(QLatin1Char('\n'), 0, 0).contains(kSep) ? kSep : QLatin1Char(',');
    std::vector<QStringList> rows;
    QStringList row;
    QString field;
    bool quoted = false;

    auto endRow = [&rows, &row]() {
        if (!(row.size() == 1 && row.first().trimmed().isEmpty()) && !row.isEmpty())
            rows.push_back(row);
        row.clear();
    };

    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (quoted) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                field += c;
            }
        } else if (c == QLatin1Char('"')) {
            quoted = true;
        } else if (c == delimiter) {
            row << field;
            field.clear();
        } else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
            if (c == QLatin1Char('\r') && i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n'))
                ++i;
            row << field;
            field.clear();
            endRow();
        } else {
            field += c;
        }
    }
    if (!field.isEmpty() || !row.isEmpty()) {
        row << field;
        endRow();
    }
    return rows;
}

QString rateLabel(Reports::Rate rate)
{
    switch (rate) {
    case Reports::Rate::Presence: return QStringLiteral("Taux de présentéisme");
    case Reports::Rate::Absence: return QStringLiteral("Taux d'absence");
    case Reports::Rate::Late: return QStringLiteral("Taux de retard");
    }
    return {};
}

QString rateFormula(Reports::Rate rate)
{
    switch (rate) {
    case Reports::Rate::Presence: return QStringLiteral("(présents + retards) / jours travaillés");
    case Reports::Rate::Absence: return QStringLiteral("absences / jours travaillés");
    case Reports::Rate::Late: return QStringLiteral("retards / (présents + retards)");
    }
    return {};
}

}

namespace Reports {

double Stats::presenceRate() const
{
    return workingDays > 0 ? static_cast<double>(present + late) / workingDays : 0.0;
}

double Stats::absenceRate() const
{
    return workingDays > 0 ? static_cast<double>(absent) / workingDays : 0.0;
}

double Stats::lateRate() const
{
    return (present + late) > 0 ? static_cast<double>(late) / (present + late) : 0.0;
}

Stats computeStats(const CloudMarks &marks, int year, int month, int restMask, const QDate &today)
{
    Stats stats;
    const int days = QDate(year, month, 1).daysInMonth();
    for (int day = 1; day <= days; ++day) {
        const QDate date(year, month, day);
        if (date > today || isRest(restMask, date))
            continue;
        ++stats.workingDays;
        const QChar status = marks.value(day);
        if (status == QChar('P'))
            ++stats.present;
        else if (status == QChar('A'))
            ++stats.absent;
        else if (status == QChar('R'))
            ++stats.late;
    }
    stats.unmarked = stats.workingDays - stats.present - stats.absent - stats.late;
    if (stats.unmarked < 0)
        stats.unmarked = 0;
    return stats;
}

QString monthlyDetailCsv(const CloudPerson &person, const CloudMarks &marks, int year, int month,
                         int restMask, const QDate &today)
{
    const QLocale french(QLocale::French);
    const Stats stats = computeStats(marks, year, month, restMask, today);
    const int days = QDate(year, month, 1).daysInMonth();

    QStringList lines;
    lines << csvLine({QStringLiteral("Nom"), person.name});
    lines << csvLine({QStringLiteral("Code"), person.code});
    lines << csvLine({QStringLiteral("Mois"), french.monthName(month) + QLatin1Char(' ') + QString::number(year)});
    lines << QString();
    lines << csvLine({QStringLiteral("Date"), QStringLiteral("Jour"), QStringLiteral("Statut")});
    for (int day = 1; day <= days; ++day) {
        const QDate date(year, month, day);
        QString status;
        const QChar mark = marks.value(day);
        if (isRest(restMask, date))
            status = QStringLiteral("Repos");
        else if (date > today)
            status = QStringLiteral("À venir");
        else if (mark == QChar('P'))
            status = QStringLiteral("Présent");
        else if (mark == QChar('A'))
            status = QStringLiteral("Absent");
        else if (mark == QChar('R'))
            status = QStringLiteral("Retard");
        else
            status = QStringLiteral("Non pointé");
        lines << csvLine({date.toString(QStringLiteral("dd/MM/yyyy")), french.dayName(date.dayOfWeek(), QLocale::LongFormat), status});
    }
    lines << QString();
    lines << csvLine({QStringLiteral("Jours travaillés"), QString::number(stats.workingDays)});
    lines << csvLine({QStringLiteral("Présents"), QString::number(stats.present)});
    lines << csvLine({QStringLiteral("Retards"), QString::number(stats.late)});
    lines << csvLine({QStringLiteral("Absents"), QString::number(stats.absent)});
    lines << csvLine({QStringLiteral("Non pointés"), QString::number(stats.unmarked)});
    lines << csvLine({QStringLiteral("Taux de présentéisme"), percent(stats.presenceRate())});
    lines << csvLine({QStringLiteral("Taux d'absence"), percent(stats.absenceRate())});
    lines << csvLine({QStringLiteral("Taux de retard"), percent(stats.lateRate())});
    return bom() + lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString rateCsv(const CloudPerson &person, const CloudMarks &marks, int year, int month, int restMask,
                const QDate &today, Rate rate)
{
    const QLocale french(QLocale::French);
    const Stats stats = computeStats(marks, year, month, restMask, today);
    double value = 0.0;
    switch (rate) {
    case Rate::Presence: value = stats.presenceRate(); break;
    case Rate::Absence: value = stats.absenceRate(); break;
    case Rate::Late: value = stats.lateRate(); break;
    }

    QStringList lines;
    lines << csvLine({QStringLiteral("Nom"), person.name});
    lines << csvLine({QStringLiteral("Code"), person.code});
    lines << csvLine({QStringLiteral("Mois"), french.monthName(month) + QLatin1Char(' ') + QString::number(year)});
    lines << csvLine({QStringLiteral("Jours travaillés"), QString::number(stats.workingDays)});
    lines << csvLine({QStringLiteral("Indicateur"), rateLabel(rate)});
    lines << csvLine({QStringLiteral("Formule"), rateFormula(rate)});
    lines << csvLine({QStringLiteral("Valeur"), percent(value)});
    return bom() + lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString rosterCsv(const std::vector<CloudPerson> &people, int kind)
{
    const KindDef &def = kindDef(kind);
    QStringList header{QStringLiteral("Code")};
    for (const FieldDef &field : def.fields)
        header << field.label;

    QStringList lines{csvLine(header)};
    for (const CloudPerson &person : people) {
        if (person.kind != kind)
            continue;
        QStringList row{person.code};
        for (size_t f = 0; f < def.fields.size(); ++f)
            row << (f < static_cast<size_t>(person.fields.size()) ? person.fields.at(static_cast<int>(f)) : QString());
        lines << csvLine(row);
    }
    return bom() + lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

RosterImport rosterFromCsv(const QString &content, int kind, const QStringList &existingCodes)
{
    RosterImport result;
    const std::vector<QStringList> rows = parseCsv(content);
    if (rows.size() < 2) {
        result.errors << QStringLiteral("Fichier vide ou sans données.");
        return result;
    }

    const KindDef &def = kindDef(kind);
    const QStringList &header = rows.front();
    int codeColumn = -1;
    std::vector<int> fieldColumn(def.fields.size(), -1);
    for (int c = 0; c < header.size(); ++c) {
        const QString name = header.at(c).trimmed().toLower();
        if (codeColumn < 0 && (name == QLatin1String("code") || name == def.idLabel.toLower()))
            codeColumn = c;
        for (size_t f = 0; f < def.fields.size(); ++f) {
            if (fieldColumn[f] < 0 && name == def.fields[f].label.toLower())
                fieldColumn[f] = c;
        }
    }
    if (codeColumn < 0) {
        result.errors << QStringLiteral("Colonne « Code » introuvable dans la première ligne.");
        return result;
    }

    QStringList seen = existingCodes;
    for (size_t r = 1; r < rows.size(); ++r) {
        const QStringList &row = rows[r];
        const int line = static_cast<int>(r) + 1;
        auto cell = [&row](int c) {
            return (c >= 0 && c < row.size()) ? row.at(c).trimmed() : QString();
        };

        const QString code = cell(codeColumn);
        if (code.isEmpty()) {
            result.errors << QStringLiteral("Ligne %1 : code manquant.").arg(line);
            continue;
        }
        if (existingCodes.contains(code)) {
            ++result.skippedExisting;
            continue;
        }
        if (!isValidCode(code)) {
            result.errors << QStringLiteral("Ligne %1 : code « %2 » invalide.").arg(line).arg(code);
            continue;
        }
        if (seen.contains(code)) {
            result.errors << QStringLiteral("Ligne %1 : code « %2 » en double.").arg(line).arg(code);
            continue;
        }

        CloudPerson person;
        person.kind = kind;
        person.code = code;
        bool valid = true;
        for (size_t f = 0; f < def.fields.size(); ++f) {
            const FieldDef &field = def.fields[f];
            QString value = cell(fieldColumn[f]);
            if (value.isEmpty()) {
                if (field.required) {
                    result.errors << QStringLiteral("Ligne %1 : « %2 » obligatoire.").arg(line).arg(field.label);
                    valid = false;
                }
                person.fields << QString();
                continue;
            }
            switch (field.type) {
            case FieldType::Email:
                if (!isValidEmail(value)) {
                    result.errors << QStringLiteral("Ligne %1 : email invalide (%2).").arg(line).arg(field.label);
                    valid = false;
                }
                break;
            case FieldType::Phone:
                if (!isValidPhone(value)) {
                    result.errors << QStringLiteral("Ligne %1 : numéro invalide (%2).").arg(line).arg(field.label);
                    valid = false;
                }
                break;
            case FieldType::Date:
                value = normalizeDate(value);
                if (value.isEmpty()) {
                    result.errors << QStringLiteral("Ligne %1 : date invalide (%2).").arg(line).arg(field.label);
                    valid = false;
                }
                break;
            case FieldType::Text:
            case FieldType::Choice:
                break;
            }
            person.fields << value;
        }
        if (!valid)
            continue;

        person.name = person.fields.value(0) + QLatin1Char(' ') + person.fields.value(1);
        seen << code;
        result.people.push_back(person);
    }
    return result;
}

QString decode(const QByteArray &bytes)
{
    QStringDecoder utf8(QStringDecoder::Utf8);
    const QString text = utf8(bytes);
    if (!utf8.hasError())
        return text;
    QStringDecoder fallback(QStringDecoder::System);
    return fallback(bytes);
}

}
