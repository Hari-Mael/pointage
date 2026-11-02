#include "persondialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

#include "fields.h"

namespace {

QWidget *makeInput(const FieldDef &field, QWidget *parent)
{
    if (field.type == FieldType::Choice) {
        auto *box = new QComboBox(parent);
        box->addItem(QString());
        box->addItems(field.choices);
        return box;
    }
    auto *edit = new QLineEdit(parent);
    if (field.type == FieldType::Date)
        edit->setPlaceholderText(QStringLiteral("JJ/MM/AAAA"));
    return edit;
}

QString valueOf(const QWidget *input)
{
    if (const auto *box = qobject_cast<const QComboBox *>(input))
        return box->currentText();
    if (const auto *edit = qobject_cast<const QLineEdit *>(input))
        return edit->text().trimmed();
    return {};
}

}

PersonDialog::PersonDialog(int kind, const QString &suggestedCode, const QStringList &existingCodes, QWidget *parent)
    : QDialog(parent), m_kind(kind), m_existingCodes(existingCodes)
{
    const KindDef &def = kindDef(kind);
    setWindowTitle(tr("Fiche %1").arg(def.singular));
    resize(460, 620);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_code = new QLineEdit(suggestedCode, this);
    form->addRow(def.idLabel + QStringLiteral(" *"), m_code);

    QString currentSection;
    for (const FieldDef &field : def.fields) {
        if (field.section != currentSection) {
            currentSection = field.section;
            form->addRow(new QLabel(QStringLiteral("<b>%1</b>").arg(currentSection), this));
        }
        QWidget *input = makeInput(field, this);
        m_inputs.push_back(input);
        form->addRow(field.label + (field.required ? QStringLiteral(" *") : QString()), input);
    }

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &PersonDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PersonDialog::validateAndAccept()
{
    const KindDef &def = kindDef(m_kind);

    const QString code = m_code->text().trimmed();
    if (!isValidCode(code)) {
        QMessageBox::warning(this, windowTitle(), tr("Identifiant invalide (1 à 16 caractères : lettres, chiffres, - et _)."));
        return;
    }
    if (m_existingCodes.contains(code)) {
        QMessageBox::warning(this, windowTitle(), tr("Cet identifiant existe déjà."));
        return;
    }

    QStringList values;
    for (size_t i = 0; i < def.fields.size(); ++i) {
        const FieldDef &field = def.fields[i];
        QString value = valueOf(m_inputs[i]);

        if (value.isEmpty()) {
            if (field.required) {
                QMessageBox::warning(this, windowTitle(), tr("Champ obligatoire : %1").arg(field.label));
                return;
            }
            values << QString();
            continue;
        }

        switch (field.type) {
        case FieldType::Email:
            if (!isValidEmail(value)) {
                QMessageBox::warning(this, windowTitle(), tr("Email invalide : %1").arg(field.label));
                return;
            }
            break;
        case FieldType::Phone:
            if (!isValidPhone(value)) {
                QMessageBox::warning(this, windowTitle(), tr("Numéro invalide : %1").arg(field.label));
                return;
            }
            break;
        case FieldType::Date:
            value = normalizeDate(value);
            if (value.isEmpty()) {
                QMessageBox::warning(this, windowTitle(), tr("Date invalide (JJ/MM/AAAA) : %1").arg(field.label));
                return;
            }
            break;
        case FieldType::Text:
        case FieldType::Choice:
            break;
        }
        values << value;
    }

    const QString orderError = checkDateOrder(m_kind, values);
    if (!orderError.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), orderError);
        return;
    }

    m_person.code = code;
    m_person.name = values.value(0) + QLatin1Char(' ') + values.value(1);
    m_person.fields = values;
    accept();
}
