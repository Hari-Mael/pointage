#pragma once

#include <QChar>
#include <QString>
#include <QStringList>
#include <vector>

enum class FieldType { Text, Email, Phone, Date, Choice };

struct FieldDef {
    QString label;
    FieldType type;
    bool required;
    QStringList choices;
    QString section;
};

struct KindDef {
    QString singular;
    QString plural;
    QChar prefix;
    QString idLabel;
    std::vector<FieldDef> fields;
};

constexpr int kKindEmployee = 0;
constexpr int kKindStudent = 1;
constexpr int kEmpHireDate = 9;
constexpr int kEmpProbationEnd = 12;
constexpr int kEmpRegularization = 13;
constexpr int kStuBirthDate = 2;

const std::vector<KindDef> &allKinds();
const KindDef &kindDef(int kind);

bool isValidEmail(const QString &value);
bool isValidPhone(const QString &value);
bool isValidCode(const QString &value);
QString normalizeDate(const QString &value);
QString checkDateOrder(int kind, const QStringList &values);
