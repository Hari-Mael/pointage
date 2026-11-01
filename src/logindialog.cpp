#include "logindialog.h"

#include <QDateTime>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "auth.h"
#include "fields.h"

namespace {

constexpr int kMinPasswordLength = 8;
constexpr int kMaxFailedLogins = 5;
constexpr qint64 kLockSeconds = 15 * 60;

}

LoginDialog::LoginDialog(Database &db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(tr("Pointage"));
    resize(420, 520);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("card"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 24, 20, 20);
    cardLayout->setSpacing(12);

    auto *icon = new QLabel(QStringLiteral("📋"), card);
    icon->setObjectName(QStringLiteral("icon"));
    icon->setAlignment(Qt::AlignCenter);
    auto *welcome = new QLabel(tr("Feuille de Pointage Pro"), card);
    welcome->setObjectName(QStringLiteral("welcome"));
    welcome->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(tr("Connectez-vous pour continuer"), card);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(icon);
    cardLayout->addWidget(welcome);
    cardLayout->addWidget(subtitle);

    auto *tabs = new QTabWidget(card);

    auto *loginPage = new QWidget(tabs);
    auto *loginForm = new QFormLayout(loginPage);
    m_loginEmail = new QLineEdit(loginPage);
    m_loginPassword = new QLineEdit(loginPage);
    m_loginPassword->setEchoMode(QLineEdit::Password);
    loginForm->addRow(tr("Email"), m_loginEmail);
    loginForm->addRow(tr("Mot de passe"), m_loginPassword);
    auto *loginButton = new QPushButton(tr("Se connecter"), loginPage);
    loginButton->setProperty("variant", QStringLiteral("blue"));
    loginForm->addRow(loginButton);
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::tryLogin);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::tryLogin);
    tabs->addTab(loginPage, tr("Connexion"));

    auto *registerPage = new QWidget(tabs);
    auto *registerForm = new QFormLayout(registerPage);
    m_regName = new QLineEdit(registerPage);
    m_regEmail = new QLineEdit(registerPage);
    m_regPassword = new QLineEdit(registerPage);
    m_regPassword->setEchoMode(QLineEdit::Password);
    m_regConfirm = new QLineEdit(registerPage);
    m_regConfirm->setEchoMode(QLineEdit::Password);
    registerForm->addRow(tr("Nom complet"), m_regName);
    registerForm->addRow(tr("Email"), m_regEmail);
    registerForm->addRow(tr("Mot de passe (8 caractères min.)"), m_regPassword);
    registerForm->addRow(tr("Confirmer"), m_regConfirm);
    auto *registerButton = new QPushButton(tr("Créer le compte"), registerPage);
    registerButton->setProperty("variant", QStringLiteral("teal"));
    registerForm->addRow(registerButton);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::tryRegister);
    tabs->addTab(registerPage, tr("Créer un compte"));

    cardLayout->addWidget(tabs);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->addWidget(card);
}

void LoginDialog::tryLogin()
{
    const QString email = m_loginEmail->text().trimmed().toLower();
    const auto user = m_db.findUser(email);
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    if (user && user->lockedUntil > now) {
        const qint64 minutes = (user->lockedUntil - now + 59) / 60;
        QMessageBox::warning(this, windowTitle(),
                             tr("Compte verrouillé après trop d'échecs. Réessayez dans %1 minute(s).").arg(minutes));
        return;
    }

    if (!user || !Auth::verifyPassword(m_loginPassword->text(), user->passwordHash)) {
        if (user) {
            const int failed = user->failedLogins + 1;
            if (failed >= kMaxFailedLogins)
                m_db.recordFailedLogin(user->id, 0, now + kLockSeconds);
            else
                m_db.recordFailedLogin(user->id, failed, 0);
        }
        QMessageBox::warning(this, windowTitle(), tr("Email ou mot de passe incorrect."));
        m_loginPassword->clear();
        return;
    }

    m_db.resetFailedLogins(user->id);
    m_user = *user;
    accept();
}

void LoginDialog::tryRegister()
{
    const QString name = m_regName->text().trimmed();
    const QString email = m_regEmail->text().trimmed().toLower();
    const QString password = m_regPassword->text();

    if (name.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Nom requis."));
        return;
    }
    if (!isValidEmail(email)) {
        QMessageBox::warning(this, windowTitle(), tr("Email invalide."));
        return;
    }
    if (password.size() < kMinPasswordLength) {
        QMessageBox::warning(this, windowTitle(), tr("Mot de passe trop court (8 caractères minimum)."));
        return;
    }
    if (password != m_regConfirm->text()) {
        QMessageBox::warning(this, windowTitle(), tr("Les mots de passe ne correspondent pas."));
        return;
    }
    if (m_db.findUser(email)) {
        QMessageBox::warning(this, windowTitle(), tr("Cet email est déjà utilisé."));
        return;
    }
    if (!m_db.createUser(email, name, Auth::hashPassword(password))) {
        QMessageBox::warning(this, windowTitle(), tr("Impossible de créer le compte."));
        return;
    }

    const auto user = m_db.findUser(email);
    if (!user)
        return;
    m_user = *user;
    accept();
}
