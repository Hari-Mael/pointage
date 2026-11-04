#include "presencedialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PresenceDialog::PresenceDialog(Mode mode, const QString &personName, int year, int month, const MarkMap &marks,
                               QWidget *parent)
    : QDialog(parent), m_mode(mode), m_year(year), m_month(month)
{
    const QLocale french(QLocale::French);
    const QDate first(year, month, 1);
    const QDate last(year, month, first.daysInMonth());

    setWindowTitle(mode == Mode::Insert ? tr("Insérer") : tr("Modifier"));
    resize(480, 380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("%1 — %2").arg(windowTitle(), personName), this);
    title->setObjectName(QStringLiteral("title"));
    layout->addWidget(title);

    auto *statusRow = new QHBoxLayout;
    m_present = new QPushButton(tr("Présence"), this);
    m_present->setCheckable(true);
    m_present->setProperty("variant", QStringLiteral("presentChoice"));
    m_absent = new QPushButton(tr("Absence"), this);
    m_absent->setCheckable(true);
    m_absent->setProperty("variant", QStringLiteral("absentChoice"));
    m_late = new QPushButton(tr("Retard"), this);
    m_late->setCheckable(true);
    m_late->setProperty("variant", QStringLiteral("choice"));
    statusRow->addWidget(m_present);
    statusRow->addWidget(m_absent);
    statusRow->addWidget(m_late);
    connect(m_present, &QPushButton::clicked, this, [this]() { setChoice(QChar('P')); });
    connect(m_absent, &QPushButton::clicked, this, [this]() { setChoice(QChar('A')); });
    connect(m_late, &QPushButton::clicked, this, [this]() { setChoice(QChar('R')); });

    if (mode == Mode::Modify) {
        m_clear = new QPushButton(tr("Effacer"), this);
        m_clear->setCheckable(true);
        m_clear->setProperty("variant", QStringLiteral("choice"));
        statusRow->addWidget(m_clear);
        connect(m_clear, &QPushButton::clicked, this, [this]() { setChoice(QChar()); });
    }
    layout->addLayout(statusRow);

    if (mode == Mode::Insert) {
        auto *dateRow = new QHBoxLayout;
        m_from = new QDateEdit(first, this);
        m_to = new QDateEdit(first, this);
        for (QDateEdit *edit : {m_from, m_to}) {
            edit->setCalendarPopup(true);
            edit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
            edit->setMinimumDate(first);
            edit->setMaximumDate(last);
        }
        dateRow->addWidget(new QLabel(tr("Du"), this));
        dateRow->addWidget(m_from, 1);
        dateRow->addWidget(new QLabel(tr("au"), this));
        dateRow->addWidget(m_to, 1);
        layout->addLayout(dateRow);
        setChoice(QChar('P'));
    } else {
        m_day = new QComboBox(this);
        for (auto it = marks.cbegin(); it != marks.cend(); ++it) {
            const QDate date(year, month, it.key());
            QString label = french.toString(date, QStringLiteral("dddd d MMMM"));
            if (it.value() == QChar('P'))
                label += tr(" — Présence");
            else if (it.value() == QChar('A'))
                label += tr(" — Absence");
            else
                label += tr(" — Retard");
            m_day->addItem(label, it.key());
        }
        layout->addWidget(m_day);
    }

    layout->addStretch(1);

    auto *buttons = new QHBoxLayout;
    auto *cancel = new QPushButton(tr("Annuler"), this);
    cancel->setProperty("variant", QStringLiteral("ghost"));
    auto *save = new QPushButton(tr("Enregistrer"), this);
    save->setProperty("variant", QStringLiteral("teal"));
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &PresenceDialog::onSave);
}

void PresenceDialog::setChoice(QChar status)
{
    m_status = status;
    m_chosen = true;
    m_present->setChecked(status == QChar('P'));
    m_absent->setChecked(status == QChar('A'));
    m_late->setChecked(status == QChar('R'));
    if (m_clear)
        m_clear->setChecked(status.isNull());
}

QList<QDate> PresenceDialog::dates() const
{
    QList<QDate> out;
    if (m_mode == Mode::Modify) {
        out << QDate(m_year, m_month, m_day->currentData().toInt());
        return out;
    }
    for (QDate day = m_from->date(); day <= m_to->date(); day = day.addDays(1))
        out << day;
    return out;
}

void PresenceDialog::onSave()
{
    if (!m_chosen) {
        QMessageBox::warning(this, windowTitle(), tr("Choisis Présence, Absence, Retard ou Effacer."));
        return;
    }
    if (m_mode == Mode::Insert && m_from->date() > m_to->date()) {
        QMessageBox::warning(this, windowTitle(), tr("La date de début doit être avant la date de fin."));
        return;
    }
    accept();
}
