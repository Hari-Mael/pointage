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
QLabel#appName { color: #f4fbf8; font-size: 12pt; font-weight: 800; }
QPushButton#pill { background: rgba(255,255,255,0.08); color: #eaf6f1; border: 1px solid rgba(255,255,255,0.14); border-radius: 16px; padding: 8px 16px; font-weight: 600; }
QPushButton#pill:hover { background: rgba(255,255,255,0.16); }
QPushButton#pillAccent { background: #34d399; color: #04140e; border: none; border-radius: 16px; padding: 8px 16px; font-weight: 700; }
QPushButton#pillAccent:hover { background: #5ee0ad; }
QFrame#profile { background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; }
QLabel#profileName { color: #f4fbf8; font-weight: 700; }
QLabel#profileMail { color: #9fbfb4; font-size: 9pt; }
QFrame#banner { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(52,211,153,0.35), stop:1 rgba(245,176,65,0.30)); border: 1px solid rgba(255,255,255,0.14); border-radius: 22px; }
QLabel#bannerKicker { color: #d1fae5; font-size: 9pt; font-weight: 700; }
QLabel#bannerTitle { color: #ffffff; font-size: 20pt; font-weight: 800; }
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

    auto *dashboard = new QPushButton(tr("Tableau de bord"), topBar);
    dashboard->setObjectName(QStringLiteral("pillAccent"));
    top->addWidget(dashboard);
    top->addStretch(1);

    auto *home = new QPushButton(tr("Accueil"), topBar);
    home->setObjectName(QStringLiteral("pill"));
    top->addWidget(home);

    m_bell = new QPushButton(QStringLiteral("🔔"), topBar);
    m_bell->setObjectName(QStringLiteral("pill"));
    m_bell->setToolTip(tr("Modifications en attente d'envoi"));
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
    titles->addWidget(kicker);
    titles->addWidget(m_school);
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
