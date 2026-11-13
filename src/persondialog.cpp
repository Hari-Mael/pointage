#include "persondialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "fields.h"
#include "photo.h"

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
    resize(540, 720);
    setMinimumSize(480, 480);

    auto *layout = new QVBoxLayout(this);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(4, 4, 12, 4);
    scroll->setWidget(content);
    layout->addWidget(scroll, 1);

    auto *photoRow = new QHBoxLayout;
    m_preview = new QLabel(this);
    m_preview->setFixedSize(96, 96);
    m_preview->setAlignment(Qt::AlignCenter);
    auto *photoButtons = new QVBoxLayout;
    auto *choose = new QPushButton(tr("Ajouter une photo (facultatif)"), this);
    choose->setProperty("variant", QStringLiteral("ghost"));
    auto *remove = new QPushButton(tr("Retirer la photo"), this);
    remove->setProperty("variant", QStringLiteral("ghost"));
    photoButtons->addWidget(choose);
    photoButtons->addWidget(remove);
    photoButtons->addStretch(1);
    photoRow->addWidget(m_preview);
    photoRow->addLayout(photoButtons, 1);
    contentLayout->addLayout(photoRow);
    connect(choose, &QPushButton::clicked, this, &PersonDialog::choosePhoto);
    connect(remove, &QPushButton::clicked, this, &PersonDialog::removePhoto);

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
        if (auto *edit = qobject_cast<QLineEdit *>(input))
            connect(edit, &QLineEdit::textChanged, this, &PersonDialog::refreshPreview);
    }
    contentLayout->addLayout(form);
    contentLayout->addStretch(1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &PersonDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    refreshPreview();
}

void PersonDialog::setPhoto(const QString &encoded)
{
    m_photo = encoded;
    refreshPreview();
}

void PersonDialog::choosePhoto()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Choisir une photo"), QString(),
                                                      tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty())
        return;
    QString error;
    const QString encoded = Photo::encodeFile(path, error);
    if (encoded.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Photo non utilisable : %1").arg(error));
        return;
    }
    m_photo = encoded;
    refreshPreview();
}

void PersonDialog::removePhoto()
{
    m_photo.clear();
    refreshPreview();
}

void PersonDialog::refreshPreview()
{
    const QString name = valueOf(m_inputs.size() > 0 ? m_inputs[0] : nullptr) + QLatin1Char(' ')
                         + valueOf(m_inputs.size() > 1 ? m_inputs[1] : nullptr);
    m_preview->setPixmap(Photo::avatar(m_photo, name.trimmed(), 96));
}

void PersonDialog::setEditMode()
{
    setWindowTitle(tr("Modifier la fiche %1").arg(kindDef(m_kind).singular));
}

void PersonDialog::setValues(const QStringList &values)
{
    for (size_t i = 0; i < m_inputs.size() && i < static_cast<size_t>(values.size()); ++i) {
        const QString value = values.at(static_cast<int>(i));
        QWidget *input = m_inputs[i];
        if (auto *box = qobject_cast<QComboBox *>(input))
            box->setCurrentText(value);
        else if (auto *edit = qobject_cast<QLineEdit *>(input))
            edit->setText(value);
    }
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
