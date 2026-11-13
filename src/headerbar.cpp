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
QFrame#topBar { background: #0b1220; border: 1px solid rgba(255,255,255,0.06); border-radius: 18px; }
QLabel#appName { color: #e8ecf2; font-size: 12pt; font-weight: 800; }
QPushButton#pill { background: #161e2e; color: #e8ecf2; border: 1px solid #243047; border-radius: 16px; padding: 8px 16px; font-weight: 600; }
QPushButton#pill:hover { background: #1d2740; }
QPushButton#pillAccent { background: #22c3b8; color: #06201d; border: none; border-radius: 16px; padding: 8px 16px; font-weight: 700; }
QFrame#profile { background: #161e2e; border-radius: 18px; }
QLabel#profileName { color: #f4f6fa; font-weight: 700; }
QLabel#profileMail { color: #8a93a0; font-size: 9pt; }
QFrame#banner { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #163a7a, stop:1 #2f6fd0); border-radius: 22px; }
QLabel#bannerKicker { color: #9fc3ff; font-size: 9pt; font-weight: 700; }
QLabel#bannerTitle { color: #ffffff; font-size: 20pt; font-weight: 800; }
QLabel#bannerSub { color: #d4e4ff; }
QComboBox#bannerCombo, QSpinBox#bannerSpin { background: rgba(255,255,255,0.12); color: #ffffff; border: 1px solid rgba(255,255,255,0.25); border-radius: 12px; padding: 6px 10px; }
QPushButton#bannerButton { background: rgba(255,255,255,0.16); color: #ffffff; border: 1px solid rgba(255,255,255,0.35); border-radius: 16px; padding: 8px 16px; font-weight: 600; }
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
    top->setContentsMargins(16, 10, 16, 10);
    top->setSpacing(10);

    auto *appName = new QLabel(QStringLiteral("📋 Feuille de Pointage Pro"), topBar);
    appName->setObjectName(QStringLiteral("appName"));
    top->addWidget(appName);
    top->addStretch(1);

    auto *home = new QPushButton(tr("Accueil"), topBar);
    home->setObjectName(QStringLiteral("pill"));
    auto *dashboard = new QPushButton(tr("Tableau de bord"), topBar);
    dashboard->setObjectName(QStringLiteral("pillAccent"));
    top->addWidget(home);
    top->addWidget(dashboard);

    m_bell = new QPushButton(QStringLiteral("🔔"), topBar);
    m_bell->setObjectName(QStringLiteral("pill"));
    m_bell->setToolTip(tr("Modifications en attente d'envoi"));
    top->addWidget(m_bell);

    auto *settings = new QPushButton(tr("Paramètres"), topBar);
    settings->setObjectName(QStringLiteral("pill"));
    top->addWidget(settings);

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

    auto *banner = new QFrame(this);
    banner->setObjectName(QStringLiteral("banner"));
    auto *bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(24, 18, 24, 18);
    bannerLayout->setSpacing(16);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    auto *kicker = new QLabel(tr("FEUILLE DE POINTAGE"), banner);
    kicker->setObjectName(QStringLiteral("bannerKicker"));
    m_school = new QLabel(SchoolInfo::name(), banner);
    m_school->setObjectName(QStringLiteral("bannerTitle"));
    m_subtitle = new QLabel(banner);
    m_subtitle->setObjectName(QStringLiteral("bannerSub"));
    titles->addWidget(kicker);
    titles->addWidget(m_school);
    titles->addWidget(m_subtitle);
    bannerLayout->addLayout(titles, 1);

    const QLocale french(QLocale::French);
    m_month = new QComboBox(banner);
    m_month->setObjectName(QStringLiteral("bannerCombo"));
    for (int month = 1; month <= 12; ++month)
        m_month->addItem(french.monthName(month));
    m_year = new QSpinBox(banner);
    m_year->setObjectName(QStringLiteral("bannerSpin"));
    m_year->setRange(2000, 2100);
    bannerLayout->addWidget(m_month);
    bannerLayout->addWidget(m_year);

    auto *schoolSettings = new QPushButton(tr("Voir les paramètres"), banner);
    schoolSettings->setObjectName(QStringLiteral("bannerButton"));
    bannerLayout->addWidget(schoolSettings);
    root->addWidget(banner);

    connect(home, &QPushButton::clicked, this, &HeaderBar::homeRequested);
    connect(dashboard, &QPushButton::clicked, this, &HeaderBar::dashboardRequested);
    connect(settings, &QPushButton::clicked, this, &HeaderBar::settingsRequested);
    connect(schoolSettings, &QPushButton::clicked, this, &HeaderBar::settingsRequested);
    connect(m_bell, &QPushButton::clicked, this, &HeaderBar::homeRequested);
}

void HeaderBar::setAvatar(const QPixmap &avatar)
{
    m_avatar->setIcon(QIcon(avatar));
}

void HeaderBar::setPendingCount(int count)
{
    m_bell->setText(count > 0 ? QStringLiteral("🔔 %1").arg(count) : QStringLiteral("🔔"));
    m_bell->setToolTip(count > 0 ? tr("%1 modification(s) en attente d'envoi").arg(count)
                                 : tr("Tout est synchronisé"));
}

void HeaderBar::setSubtitle(const QString &text)
{
    m_subtitle->setText(text);
}
