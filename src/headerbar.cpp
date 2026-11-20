#include "headerbar.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

#include "schoolinfo.h"

namespace {

const char *const kHeaderStyle = R"(
QFrame#topBar { background: rgba(255,255,255,0.07); border: 1px solid rgba(255,255,255,0.14); border-radius: 18px; }
QFrame#profile { background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; }
QLabel#profileName { color: #f4fbf8; font-weight: 700; }
QLabel#profileMail { color: #9fbfb4; font-size: 9pt; }
QLabel#bannerTitle { color: #ffffff; font-size: 14pt; font-weight: 800; }
QPushButton#menuToggle { font-size: 16pt; font-weight: 800; padding: 0px; min-width: 40px; max-width: 40px; min-height: 36px; max-height: 36px; }
QLineEdit#headerSearch { background: rgba(0,0,0,0.28); color: #eaf6f1; border: 1px solid rgba(255,255,255,0.12); border-radius: 14px; padding: 8px 14px; }
QLineEdit#headerSearch:focus { border: 1px solid #34d399; }
QComboBox#bannerCombo, QSpinBox#bannerSpin { background: rgba(255,255,255,0.12); color: #ffffff; border: 1px solid rgba(255,255,255,0.25); border-radius: 12px; padding: 6px 10px; }
)";

}

HeaderBar::HeaderBar(const QString &userName, const QString &userEmail, const QPixmap &avatar, QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(QString::fromLatin1(kHeaderStyle));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);

    auto *topBar = new QFrame(this);
    topBar->setObjectName(QStringLiteral("topBar"));
    auto *top = new QHBoxLayout(topBar);
    top->setContentsMargins(12, 8, 12, 8);
    top->setSpacing(10);

    m_menuToggle = new QPushButton(QStringLiteral("⋮"), topBar);
    m_menuToggle->setObjectName(QStringLiteral("menuToggle"));
    m_menuToggle->setToolTip(tr("Afficher / masquer le menu"));
    top->addWidget(m_menuToggle);

    m_school = new QLabel(SchoolInfo::name(), topBar);
    m_school->setObjectName(QStringLiteral("bannerTitle"));
    top->addWidget(m_school);

    m_search = new QLineEdit(topBar);
    m_search->setObjectName(QStringLiteral("headerSearch"));
    m_search->setPlaceholderText(tr("🔍 Rechercher par ID ou Nom..."));
    m_search->setClearButtonEnabled(true);
    top->addWidget(m_search, 1);

    const QLocale french(QLocale::French);
    m_month = new QComboBox(topBar);
    m_month->setObjectName(QStringLiteral("bannerCombo"));
    for (int month = 1; month <= 12; ++month)
        m_month->addItem(french.monthName(month));
    m_year = new QSpinBox(topBar);
    m_year->setObjectName(QStringLiteral("bannerSpin"));
    m_year->setRange(2000, 2100);
    top->addWidget(m_month);
    top->addWidget(m_year);

    m_bell = new QPushButton(QStringLiteral("🔔"), topBar);
    m_bell->setToolTip(tr("Notifications"));
    top->addWidget(m_bell);

    auto *profile = new QFrame(topBar);
    profile->setObjectName(QStringLiteral("profile"));
    auto *profileLayout = new QHBoxLayout(profile);
    profileLayout->setContentsMargins(12, 4, 6, 4);
    profileLayout->setSpacing(10);
    auto *names = new QVBoxLayout;
    names->setSpacing(0);
    auto *name = new QLabel(userName, profile);
    name->setObjectName(QStringLiteral("profileName"));
    name->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *mail = new QLabel(userEmail, profile);
    mail->setObjectName(QStringLiteral("profileMail"));
    mail->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    names->addWidget(name);
    names->addWidget(mail);
    m_avatar = new QPushButton(profile);
    m_avatar->setFlat(true);
    m_avatar->setIconSize(QSize(40, 40));
    m_avatar->setFixedSize(44, 44);
    m_avatar->setToolTip(tr("Photo de profil"));
    m_avatar->setIcon(QIcon(avatar));
    profileLayout->addLayout(names);
    profileLayout->addWidget(m_avatar);
    top->addWidget(profile);
    root->addWidget(topBar);

    connect(m_menuToggle, &QPushButton::clicked, this, &HeaderBar::menuToggleRequested);
    connect(m_bell, &QPushButton::clicked, this, &HeaderBar::notificationsRequested);
}

void HeaderBar::setAvatar(const QPixmap &avatar)
{
    m_avatar->setIcon(QIcon(avatar));
}

void HeaderBar::setBellCount(int notifications, int pending)
{
    m_bell->setText(notifications > 0 ? QStringLiteral("🔔 %1").arg(notifications) : QStringLiteral("🔔"));
    m_bell->setToolTip(tr("%1 notification(s) — %2 modification(s) en attente d'envoi")
                           .arg(notifications)
                           .arg(pending));
}
