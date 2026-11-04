#include "logindialog.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "fields.h"
#include "secretstore.h"

namespace {

constexpr int kMinPasswordLength = 8;

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

LoginDialog::LoginDialog(const QString &dataDir, QWidget *parent)
    : QDialog(parent), m_dataDir(dataDir)
{
    setObjectName(QStringLiteral("loginDialog"));
    setWindowTitle(tr("Pointage"));
    resize(420, 640);

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

    m_stack = new QStackedWidget(card);

    auto *loginPage = new QWidget(m_stack);
    auto *loginLayout = new QVBoxLayout(loginPage);
    loginLayout->setContentsMargins(0, 0, 0, 0);
    loginLayout->setSpacing(14);
    m_loginEmail = makeField(tr("Adresse email"), false, loginPage);
    m_loginPassword = makeField(tr("Mot de passe"), true, loginPage);
    m_rememberMe = new QCheckBox(tr("Se souvenir de moi"), loginPage);
    auto *forgot = makeButton(tr("Mot de passe oublié ?"), QStringLiteral("linkButton"), loginPage);
    auto *optionsRow = new QHBoxLayout;
    optionsRow->addWidget(m_rememberMe);
    optionsRow->addStretch(1);
    optionsRow->addWidget(forgot);
    auto *loginButton = makeButton(tr("CONNEXION"), QStringLiteral("embossButton"), loginPage);
    auto *toRegister = makeButton(tr("Pas encore de compte ? Créer un compte"), QStringLiteral("linkButton"), loginPage);
    loginLayout->addWidget(m_loginEmail);
    loginLayout->addWidget(m_loginPassword);
    loginLayout->addLayout(optionsRow);
    loginLayout->addSpacing(6);
    loginLayout->addWidget(loginButton);
    loginLayout->addWidget(toRegister, 0, Qt::AlignCenter);
    loginLayout->addStretch(1);
    m_stack->addWidget(loginPage);

    auto *registerPage = new QWidget(m_stack);
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
    m_stack->addWidget(registerPage);

    cardLayout->addWidget(m_stack, 1);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::tryLogin);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::tryLogin);
    connect(forgot, &QPushButton::clicked, this, &LoginDialog::forgotPassword);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::tryRegister);
    connect(toRegister, &QPushButton::clicked, this, [this]() { m_stack->setCurrentIndex(1); });
    connect(toLogin, &QPushButton::clicked, this, [this]() { m_stack->setCurrentIndex(0); });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addWidget(card);
}

bool LoginDialog::acceptSession(const SupabaseSession &session, bool remember)
{
    m_session = session;
    if (remember)
        SecretStore::save(m_dataDir, session.refreshToken);
    else
        SecretStore::clear(m_dataDir);
    return true;
}

bool LoginDialog::restoreSession()
{
    const QString refreshToken = SecretStore::load(m_dataDir);
    if (refreshToken.isEmpty())
        return false;

    SupabaseSession session;
    QString error;
    if (!m_auth.refresh(refreshToken, session, error)) {
        SecretStore::clear(m_dataDir);
        return false;
    }
    m_session = session;
    SecretStore::save(m_dataDir, session.refreshToken);
    return true;
}

void LoginDialog::tryLogin()
{
    const QString email = m_loginEmail->text().trimmed().toLower();
    SupabaseSession session;
    QString error;
    if (!m_auth.signIn(email, m_loginPassword->text(), session, error)) {
        QMessageBox::warning(this, windowTitle(),
                             error.isEmpty() ? tr("Email ou mot de passe incorrect.") : error);
        m_loginPassword->clear();
        return;
    }
    if (acceptSession(session, m_rememberMe->isChecked()))
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

    SupabaseSession session;
    QString error;
    if (!m_auth.signUp(email, password, name, session, error)) {
        QMessageBox::warning(this, windowTitle(), error.isEmpty() ? tr("Impossible de créer le compte.") : error);
        return;
    }
    if (session.accessToken.isEmpty()) {
        QMessageBox::information(this, windowTitle(),
                                 tr("Compte créé. Ouvrez le lien reçu par email pour le confirmer, puis connectez-vous."));
        m_stack->setCurrentIndex(0);
        return;
    }
    if (acceptSession(session, false))
        accept();
}

void LoginDialog::forgotPassword()
{
    bool ok = false;
    const QString email = QInputDialog::getText(this, tr("Mot de passe oublié"),
                                                tr("Adresse email du compte :"), QLineEdit::Normal,
                                                m_loginEmail->text().trimmed(), &ok)
                              .trimmed()
                              .toLower();
    if (!ok || email.isEmpty())
        return;
    if (!isValidEmail(email)) {
        QMessageBox::warning(this, windowTitle(), tr("Email invalide."));
        return;
    }
    QString error;
    if (!m_auth.sendPasswordReset(email, error)) {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    QMessageBox::information(this, windowTitle(),
                             tr("Si un compte existe pour %1, un email de réinitialisation vient d'être envoyé.").arg(email));
}
