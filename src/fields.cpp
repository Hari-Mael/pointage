#include "fields.h"

#include <QDate>
#include <QObject>
#include <QRegularExpression>

namespace {

const QString kPersonal = QStringLiteral("Informations personnelles");
const QString kJob = QStringLiteral("Détails d'emploi");
const QString kEmergency = QStringLiteral("Contact d'urgence");
const QString kStudentIdentity = QStringLiteral("Identité de l'élève");
const QString kFather = QStringLiteral("Père");
const QString kMother = QStringLiteral("Mère");
const QString kParents = QStringLiteral("Contacts des parents");
const QString kSchool = QStringLiteral("Scolarité d'origine");

}

const std::vector<KindDef> &allKinds()
{
    static const std::vector<KindDef> kinds = {
        {QStringLiteral("employé"), QStringLiteral("Employés"), QLatin1Char('E'), QStringLiteral("ID employé"),
         {
             {QStringLiteral("Prénom"), FieldType::Text, true, {}, kPersonal},
             {QStringLiteral("Nom"), FieldType::Text, true, {}, kPersonal},
             {QStringLiteral("Poste"), FieldType::Text, false, {}, kPersonal},
             {QStringLiteral("Département"), FieldType::Text, false, {}, kPersonal},
             {QStringLiteral("Téléphone"), FieldType::Phone, false, {}, kPersonal},
             {QStringLiteral("Email"), FieldType::Email, false, {}, kPersonal},
             {QStringLiteral("Adresse"), FieldType::Text, false, {}, kPersonal},
             {QStringLiteral("Type de contrat"), FieldType::Choice, false,
              {QStringLiteral("CDI"), QStringLiteral("CDD"), QStringLiteral("Intérim"), QStringLiteral("Stage"), QStringLiteral("Freelance")},
              kPersonal},
             {QStringLiteral("Mode de travail"), FieldType::Choice, false,
              {QStringLiteral("Sur site"), QStringLiteral("Télétravail"), QStringLiteral("Hybride")},
              kPersonal},
             {QStringLiteral("Date d'embauche"), FieldType::Date, false, {}, kJob},
             {QStringLiteral("Statut"), FieldType::Choice, false,
              {QStringLiteral("Période d'essai"), QStringLiteral("Titulaire"), QStringLiteral("Suspendu"), QStringLiteral("Démissionnaire")},
              kJob},
             {QStringLiteral("Supérieur direct"), FieldType::Text, false, {}, kJob},
             {QStringLiteral("Fin de période d'essai"), FieldType::Date, false, {}, kJob},
             {QStringLiteral("Date de titularisation"), FieldType::Date, false, {}, kJob},
             {QStringLiteral("Urgence — nom"), FieldType::Text, false, {}, kEmergency},
             {QStringLiteral("Urgence — lien de parenté"), FieldType::Text, false, {}, kEmergency},
             {QStringLiteral("Urgence — téléphone"), FieldType::Phone, false, {}, kEmergency},
             {QStringLiteral("Urgence — adresse"), FieldType::Text, false, {}, kEmergency},
         }},
        {QStringLiteral("élève"), QStringLiteral("Élèves"), QLatin1Char('M'), QStringLiteral("Numéro matricule"),
         {
             {QStringLiteral("Nom"), FieldType::Text, true, {}, kStudentIdentity},
             {QStringLiteral("Prénom"), FieldType::Text, true, {}, kStudentIdentity},
             {QStringLiteral("Date de naissance"), FieldType::Date, false, {}, kStudentIdentity},
             {QStringLiteral("Lieu de naissance"), FieldType::Text, false, {}, kStudentIdentity},
             {QStringLiteral("Adresse"), FieldType::Text, false, {}, kStudentIdentity},
             {QStringLiteral("Nom du père"), FieldType::Text, false, {}, kFather},
             {QStringLiteral("Profession du père"), FieldType::Text, false, {}, kFather},
             {QStringLiteral("Contact du père"), FieldType::Phone, false, {}, kFather},
             {QStringLiteral("Nom de la mère"), FieldType::Text, false, {}, kMother},
             {QStringLiteral("Profession de la mère"), FieldType::Text, false, {}, kMother},
             {QStringLiteral("Contact de la mère"), FieldType::Phone, false, {}, kMother},
             {QStringLiteral("Contact des parents"), FieldType::Phone, false, {}, kParents},
             {QStringLiteral("Email des parents"), FieldType::Email, false, {}, kParents},
             {QStringLiteral("École d'origine"), FieldType::Text, false, {}, kSchool},
             {QStringLiteral("Classe d'origine"), FieldType::Text, false, {}, kSchool},
         }},
    };
    return kinds;
}

const KindDef &kindDef(int kind)
{
    return allKinds().at(static_cast<size_t>(kind));
}

bool isValidEmail(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
    return value.size() <= 100 && pattern.match(value).hasMatch();
}

bool isValidPhone(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9+()\\-. ]{6,24}$"));
    return pattern.match(value).hasMatch();
}

bool isValidCode(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9_-]{1,16}$"));
    return pattern.match(value).hasMatch();
}

QString normalizeDate(const QString &value)
{
    QDate date = QDate::fromString(value, QStringLiteral("dd/MM/yyyy"));
    if (!date.isValid())
        date = QDate::fromString(value, Qt::ISODate);
    if (!date.isValid() || date.year() < 1900 || date.year() > 2100)
        return {};
    return date.toString(Qt::ISODate);
}

QString checkDateOrder(int kind, const QStringList &values)
{
    if (kind == kKindEmployee) {
        const QString hire = values.value(kEmpHireDate);
        const QString probation = values.value(kEmpProbationEnd);
        const QString regularization = values.value(kEmpRegularization);
        if (!hire.isEmpty() && !probation.isEmpty() && probation < hire)
            return QObject::tr("La fin d'essai précède la date d'embauche.");
        if (!probation.isEmpty() && !regularization.isEmpty() && regularization < probation)
            return QObject::tr("La titularisation précède la fin d'essai.");
        return {};
    }
    const QString birth = values.value(kStuBirthDate);
    if (!birth.isEmpty() && birth > QDate::currentDate().toString(Qt::ISODate))
        return QObject::tr("La date de naissance est dans le futur.");
    return {};
}
