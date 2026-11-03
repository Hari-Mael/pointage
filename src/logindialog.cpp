#include "logindialog.h"

#include <QDateTime>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "auth.h"
#include "fields.h"

namespace {

constexpr int kMinPasswordLength = 8;
constexpr int kMaxFailedLogins = 5;
constexpr qint64 kLockSeconds = 15 * 60;

QLineEdit *makeField(const QString &placeholder, bool password, QWidget *parent)
{
    auto *edit = new QLineEdit(parent);
    edit->setObjectName(QStringLiteral("embossed"));
    edit->setPlaceholderText(placeholder);
    if (password)
        edit->setEchoMode(QLineEdit::Password);
    return edit;
}

QPushButton *makeButton(const QString &text, const QString &name, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(name);
    return button;
}

}

LoginDialog::LoginDialog(Database &db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setObjectName(QStringLiteral("loginDialog"));
    setWindowTitle(tr("Pointage"));
    resize(420, 600);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("loginCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(30, 30, 30, 30);
    cardLayout->setSpacing(14);

    auto *icon = new QLabel(QStringLiteral("🔐"), card);
    icon->setObjectName(QStringLiteral("iconCircle"));
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(72, 72);
    auto *iconRow = new QHBoxLayout;
    iconRow->addStretch(1);
    iconRow->addWidget(icon);
    iconRow->addStretch(1);
    cardLayout->addLayout(iconRow);

    auto *title = new QLabel(tr("Bienvenue"), card);
    title->setObjectName(QStringLiteral("welcome"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(tr("Connectez-vous pour continuer"), card);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);
    cardLayout->addWidget(subtitle);
    cardLayout->addSpacing(8);

    auto *stack = new QStackedWidget(card);

    auto *loginPage = new QWidget(stack);
    auto *loginLayout = new QVBoxLayout(loginPage);
    loginLayout->setContentsMargins(0, 0, 0, 0);
    loginLayout->setSpacing(14);
    m_loginEmail = makeField(tr("Adresse email"), false, loginPage);
    m_loginPassword = makeField(tr("Mot de passe"), true, loginPage);
    auto *loginButton = makeButton(tr("CONNEXION"), QStringLiteral("embossButton"), loginPage);
    auto *toRegister = makeButton(tr("Pas encore de compte ? Créer un compte"), QStringLiteral("linkButton"), loginPage);
    loginLayout->addWidget(m_loginEmail);
    loginLayout->addWidget(m_loginPassword);
    loginLayout->addSpacing(6);
    loginLayout->addWidget(loginButton);
    loginLayout->addWidget(toRegister, 0, Qt::AlignCenter);
    loginLayout->addStretch(1);
    stack->addWidget(loginPage);

    auto *registerPage = new QWidget(stack);
    auto *registerLayout = new QVBoxLayout(registerPage);
    registerLayout->setContentsMargins(0, 0, 0, 0);
    registerLayout->setSpacing(14);
    m_regName = makeField(tr("Nom complet"), false, registerPage);
    m_regEmail = makeField(tr("Adresse email"), false, registerPage);
    m_regPassword = makeField(tr("Mot de passe (8 caractères min.)"), true, registerPage);
    m_regConfirm = makeField(tr("Confirmer le mot de passe"), true, registerPage);
    auto *registerButton = makeButton(tr("CRÉER LE COMPTE"), QStringLiteral("embossButton"), registerPage);
    auto *toLogin = makeButton(tr("Déjà un compte ? Se connecter"), QStringLiteral("linkButton"), registerPage);
    registerLayout->addWidget(m_regName);
    registerLayout->addWidget(m_regEmail);
    registerLayout->addWidget(m_regPassword);
    registerLayout->addWidget(m_regConfirm);
    registerLayout->addSpacing(6);
    registerLayout->addWidget(registerButton);
    registerLayout->addWidget(toLogin, 0, Qt::AlignCenter);
    registerLayout->addStretch(1);
    stack->addWidget(registerPage);

    cardLayout->addWidget(stack, 1);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::tryLogin);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::tryLogin);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::tryRegister);
    connect(toRegister, &QPushButton::clicked, this, [stack]() { stack->setCurrentIndex(1); });
    connect(toLogin, &QPushButton::clicked, this, [stack]() { stack->setCurrentIndex(0); });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
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
