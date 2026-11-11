#include "settingsdialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "logo.h"
#include "privacynotice.h"

namespace {

QTimeEdit *makeTimeEdit(const QTime &time, QWidget *parent)
{
    auto *edit = new QTimeEdit(time, parent);
    edit->setDisplayFormat(QStringLiteral("HH:mm"));
    return edit;
}

QHBoxLayout *makeRange(QTimeEdit *from, QTimeEdit *to, QWidget *parent)
{
    auto *row = new QHBoxLayout;
    row->addWidget(from);
    row->addWidget(new QLabel(QObject::tr("à"), parent));
    row->addWidget(to);
    return row;
}

}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Paramètres"));
    resize(480, 560);
    const Schedule schedule = Schedule::load();

    auto *layout = new QVBoxLayout(this);

    auto *logoBox = new QGroupBox(tr("Logo"), this);
    auto *logoLayout = new QHBoxLayout(logoBox);
    auto *changeLogoButton = new QPushButton(tr("Changer le logo"), logoBox);
    changeLogoButton->setProperty("variant", QStringLiteral("ghost"));
    auto *resetLogoButton = new QPushButton(tr("Logo par défaut"), logoBox);
    resetLogoButton->setProperty("variant", QStringLiteral("ghost"));
    logoLayout->addWidget(changeLogoButton);
    logoLayout->addWidget(resetLogoButton);
    layout->addWidget(logoBox);

    auto *privacyBox = new QGroupBox(tr("Confidentialité"), this);
    auto *privacyLayout = new QHBoxLayout(privacyBox);
    auto *privacyButton = new QPushButton(tr("Voir la politique de confidentialité"), privacyBox);
    privacyButton->setProperty("variant", QStringLiteral("ghost"));
    privacyLayout->addWidget(privacyButton);
    layout->addWidget(privacyBox);

    auto *scheduleBox = new QGroupBox(tr("Créneaux horaires"), this);
    auto *form = new QFormLayout(scheduleBox);
    m_morningStart = makeTimeEdit(schedule.morningStart, this);
    m_morningEnd = makeTimeEdit(schedule.morningEnd, this);
    m_pauseStart = makeTimeEdit(schedule.pauseStart, this);
    m_pauseEnd = makeTimeEdit(schedule.pauseEnd, this);
    m_afternoonStart = makeTimeEdit(schedule.afternoonStart, this);
    m_afternoonEnd = makeTimeEdit(schedule.afternoonEnd, this);
    form->addRow(tr("Matin"), makeRange(m_morningStart, m_morningEnd, this));
    form->addRow(tr("Pause"), makeRange(m_pauseStart, m_pauseEnd, this));
    form->addRow(tr("Après-midi"), makeRange(m_afternoonStart, m_afternoonEnd, this));
    layout->addWidget(scheduleBox);

    auto *note = new QLabel(tr("Chaque période doit finir après son début. Le matin se termine au début de la pause, "
                               "et l'après-midi commence à la fin de la pause."),
                            this);
    note->setWordWrap(true);
    layout->addWidget(note);
    layout->addStretch(1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(changeLogoButton, &QPushButton::clicked, this, &SettingsDialog::changeLogo);
    connect(resetLogoButton, &QPushButton::clicked, this, &SettingsDialog::resetLogo);
    connect(privacyButton, &QPushButton::clicked, this, &SettingsDialog::openPrivacy);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::updateIcons()
{
    qApp->setWindowIcon(Logo::icon());
    if (parentWidget())
        parentWidget()->setWindowIcon(Logo::icon());
}

void SettingsDialog::changeLogo()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Choisir un logo"), QString(),
                                                      tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"));
    if (path.isEmpty())
        return;
    QString error;
    if (!Logo::setCustom(path, error)) {
        QMessageBox::warning(this, windowTitle(), tr("Logo non enregistré : %1").arg(error));
        return;
    }
    updateIcons();
}

void SettingsDialog::resetLogo()
{
    Logo::resetCustom();
    updateIcons();
}

void SettingsDialog::openPrivacy()
{
    showPrivacyNotice(this);
}

void SettingsDialog::save()
{
    Schedule schedule;
    schedule.morningStart = m_morningStart->time();
    schedule.morningEnd = m_morningEnd->time();
    schedule.pauseStart = m_pauseStart->time();
    schedule.pauseEnd = m_pauseEnd->time();
    schedule.afternoonStart = m_afternoonStart->time();
    schedule.afternoonEnd = m_afternoonEnd->time();

    if (!schedule.isValid()) {
        QMessageBox::warning(this, windowTitle(),
                             tr("Créneaux invalides : chaque période doit finir après son début, "
                                "et les périodes doivent se suivre (matin, pause, après-midi)."));
        return;
    }
    schedule.save();
    emit scheduleSaved(schedule);
    accept();
}
