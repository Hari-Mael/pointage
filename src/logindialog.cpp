#include "logindialog.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadialGradient>
#include <QStackedWidget>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include <functional>

#include "fields.h"
#include "logo.h"
#include "secretstore.h"

namespace {

constexpr int kMinPasswordLength = 8;
const QList<QColor> kLightColors{QColor("#f5a524"), QColor("#22c3b8"), QColor("#e0457b"),
                                 QColor("#4a8df0"), QColor("#9b6cf0")};

QString cardStyle(const QColor &accent)
{
    QString style = QStringLiteral(
        "QFrame#loginCard { background: rgba(22,26,34,0.95); border: 1px solid rgba(255,255,255,0.08); border-radius: 26px; }"
        "QLabel#cardTitle { color: #f4f6fa; font-size: 18pt; font-weight: 800; }"
        "QLabel#cardSub { color: #8a93a0; }"
        "QLineEdit#embossed { background: #141820; color: #e8ecf2; border: 1px solid #262d39; border-radius: 12px; padding: 11px 14px; }"
        "QLineEdit#embossed:focus { border: 1px solid @ACCENT@; }"
        "QPushButton#embossButton { background: @ACCENT@; color: #101418; border: none; border-radius: 12px; padding: 12px; font-weight: 800; }"
        "QPushButton#embossButton:hover { background: #ffffff; }"
        "QPushButton#tabButton { background: transparent; color: #8a93a0; border: none; padding: 8px 4px; font-weight: 700; border-bottom: 2px solid transparent; }"
        "QPushButton#tabButton:checked { color: @ACCENT@; border-bottom: 2px solid @ACCENT@; }"
        "QPushButton#linkButton { background: transparent; color: @ACCENT@; border: none; padding: 4px; text-align: left; }"
        "QCheckBox { color: #8a93a0; }");
    style.replace(QStringLiteral("@ACCENT@"), accent.name());
    return style;
}

}

class LampWidget : public QWidget {
public:
    explicit LampWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(420, 380);
    }

    void setLit(bool lit) { m_lit = lit; update(); }
    bool isLit() const { return m_lit; }
    void setGlow(const QColor &color) { m_glow = color; update(); }
    QColor glow() const { return m_glow; }
    void setOnPull(std::function<void()> callback) { m_onPull = std::move(callback); }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (m_onPull && cordArea().contains(event->position()))
            m_onPull();
    }

    void paintEvent(QPaintEvent *) override
    {
        const qreal s = qMin(width(), height()) / 420.0;
        const QPointF c(width() * 0.5, height() * 0.5);
        auto at = [&](qreal x, qreal y) { return c + QPointF(x * s, y * s); };

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (m_lit) {
            QRadialGradient room(at(80, -130), 340 * s);
            QColor soft = m_glow;
            soft.setAlpha(28);
            room.setColorAt(0, soft);
            soft.setAlpha(0);
            room.setColorAt(1, soft);
            p.fillRect(rect(), room);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#1a1f27"));
        p.drawEllipse(QRectF(at(0, 150).x() - 80 * s, at(0, 150).y() - 12 * s, 160 * s, 24 * s));

        QPen armPen(QColor("#1a1f27"), 12 * s, Qt::SolidLine, Qt::RoundCap);
        p.setPen(armPen);
        p.drawLine(at(0, 135), at(-30, -10));
        p.drawLine(at(-30, -10), at(80, -130));
        p.setPen(Qt::NoPen);

        const QPointF head = at(80, -130);
        p.save();
        p.translate(head);
        p.rotate(-20);
        QPainterPath shade;
        shade.moveTo(-70 * s, 0);
        shade.arcTo(QRectF(-70 * s, -60 * s, 140 * s, 120 * s), 180, -180);
        shade.closeSubpath();
        p.setBrush(QColor("#1e242e"));
        p.drawPath(shade);
        p.restore();

        const QPointF bulb = head + QPointF(0, 14 * s);
        if (m_lit) {
            QRadialGradient halo(bulb, 260 * s);
            QColor halColor = m_glow;
            halColor.setAlpha(140);
            halo.setColorAt(0, halColor);
            halColor.setAlpha(0);
            halo.setColorAt(1, halColor);
            p.setBrush(halo);
            p.drawEllipse(bulb, 260 * s, 260 * s);

            QPainterPath cone;
            cone.moveTo(bulb);
            cone.lineTo(bulb + QPointF(440 * s, -110 * s));
            cone.lineTo(bulb + QPointF(440 * s, 210 * s));
            cone.closeSubpath();
            QLinearGradient beam(bulb, bulb + QPointF(440 * s, 0));
            QColor start = m_glow;
            start.setAlpha(170);
            QColor end = m_glow;
            end.setAlpha(10);
            beam.setColorAt(0, start);
            beam.setColorAt(1, end);
            p.setBrush(beam);
            p.drawPath(cone);
        }

        p.setBrush(m_lit ? QColor("#fff6d8") : QColor("#3a414d"));
        p.drawEllipse(bulb, 13 * s, 13 * s);

        const QPointF ball = head + QPointF(50 * s, 118 * s);
        p.setPen(QPen(QColor("#2b313b"), 2 * s));
        p.drawLine(head + QPointF(50 * s, 12 * s), ball);
        p.setPen(Qt::NoPen);
        p.setBrush(m_glow);
        p.drawEllipse(ball, 7 * s, 7 * s);

        if (!m_lit) {
            p.setPen(QColor("#8a93a0"));
            p.drawText(QRectF(ball.x() - 90 * s, ball.y() + 18 * s, 180 * s, 24 * s), Qt::AlignCenter,
                       QCoreApplication::translate("LoginDialog", "Tirez le cordon"));
        }
    }

private:
    QRectF cordArea() const
    {
        const qreal s = qMin(width(), height()) / 420.0;
        const QPointF c(width() * 0.5, height() * 0.5);
        const QPointF ball = c + QPointF(80 * s + 50 * s, -130 * s + 118 * s);
        return QRectF(ball.x() - 22 * s, ball.y() - 30 * s, 44 * s, 60 * s);
    }

    bool m_lit = false;
    QColor m_glow = kLightColors.first();
    std::function<void()> m_onPull;
};

LoginDialog::LoginDialog(const QString &dataDir, QWidget *parent)
    : QDialog(parent), m_dataDir(dataDir), m_accent(kLightColors.first())
{
    setObjectName(QStringLiteral("loginDialog"));
    setWindowTitle(tr("Pointage"));
    resize(900, 580);
    setStyleSheet(QStringLiteral("QDialog#loginDialog { background: #0e1218; }"));

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);

    m_lamp = new LampWidget(this);
    m_lamp->setGlow(m_accent);
    m_lamp->setOnPull([this]() { onPull(); });
    root->addWidget(m_lamp, 3);

    m_card = new QFrame(this);
    m_card->setObjectName(QStringLiteral("loginCard"));
    m_card->setFixedWidth(390);
    m_cardEffect = new QGraphicsOpacityEffect(m_card);
    m_cardEffect->setOpacity(0.0);
    m_card->setGraphicsEffect(m_cardEffect);
    m_card->setEnabled(false);
    root->addWidget(m_card, 0, Qt::AlignVCenter);

    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(30, 24, 30, 24);
    cardLayout->setSpacing(12);

    auto *logo = new QLabel(m_card);
    logo->setPixmap(Logo::pixmap(44));
    logo->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(logo);

    auto *tabs = new QHBoxLayout;
    m_tabLogin = new QPushButton(tr("Connexion"), m_card);
    m_tabLogin->setObjectName(QStringLiteral("tabButton"));
    m_tabLogin->setCheckable(true);
    m_tabLogin->setChecked(true);
    m_tabRegister = new QPushButton(tr("Inscription"), m_card);
    m_tabRegister->setObjectName(QStringLiteral("tabButton"));
    m_tabRegister->setCheckable(true);
    tabs->addWidget(m_tabLogin);
    tabs->addWidget(m_tabRegister);
    cardLayout->addLayout(tabs);

    m_stack = new QStackedWidget(m_card);
    m_stack->addWidget(buildLoginPage());
    m_stack->addWidget(buildRegisterPage());
    cardLayout->addWidget(m_stack, 1);

    connect(m_tabLogin, &QPushButton::clicked, this, [this]() { switchMode(0); });
    connect(m_tabRegister, &QPushButton::clicked, this, [this]() { switchMode(1); });
    applyAccent(m_accent);
}

QWidget *LoginDialog::buildLoginPage()
{
    auto *page = new QWidget(m_stack);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Bienvenue"), page);
    title->setObjectName(QStringLiteral("cardTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto *sub = new QLabel(tr("Connectez-vous pour continuer"), page);
    sub->setObjectName(QStringLiteral("cardSub"));
    sub->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(sub);
    layout->addSpacing(6);

    m_loginEmail = new QLineEdit(page);
    m_loginEmail->setObjectName(QStringLiteral("embossed"));
    m_loginEmail->setPlaceholderText(tr("Adresse email"));
    m_loginPassword = new QLineEdit(page);
    m_loginPassword->setObjectName(QStringLiteral("embossed"));
    m_loginPassword->setPlaceholderText(tr("Mot de passe"));
    m_loginPassword->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_loginEmail);
    layout->addWidget(m_loginPassword);

    auto *options = new QHBoxLayout;
    m_rememberMe = new QCheckBox(tr("Se souvenir de moi"), page);
    auto *forgot = new QPushButton(tr("Mot de passe oublié ?"), page);
    forgot->setObjectName(QStringLiteral("linkButton"));
    options->addWidget(m_rememberMe);
    options->addStretch(1);
    options->addWidget(forgot);
    layout->addLayout(options);

    auto *login = new QPushButton(tr("SE CONNECTER"), page);
    login->setObjectName(QStringLiteral("embossButton"));
    layout->addWidget(login);
    layout->addStretch(1);

    connect(login, &QPushButton::clicked, this, &LoginDialog::tryLogin);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::tryLogin);
    connect(forgot, &QPushButton::clicked, this, &LoginDialog::forgotPassword);
    return page;
}

QWidget *LoginDialog::buildRegisterPage()
{
    auto *page = new QWidget(m_stack);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Créer un compte"), page);
    title->setObjectName(QStringLiteral("cardTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto *sub = new QLabel(tr("Commencez votre parcours"), page);
    sub->setObjectName(QStringLiteral("cardSub"));
    sub->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(sub);
    layout->addSpacing(6);

    m_regName = new QLineEdit(page);
    m_regName->setObjectName(QStringLiteral("embossed"));
    m_regName->setPlaceholderText(tr("Nom complet"));
    m_regEmail = new QLineEdit(page);
    m_regEmail->setObjectName(QStringLiteral("embossed"));
    m_regEmail->setPlaceholderText(tr("Adresse email"));
    m_regPassword = new QLineEdit(page);
    m_regPassword->setObjectName(QStringLiteral("embossed"));
    m_regPassword->setPlaceholderText(tr("Mot de passe (8 caractères min.)"));
    m_regPassword->setEchoMode(QLineEdit::Password);
    m_regConfirm = new QLineEdit(page);
    m_regConfirm->setObjectName(QStringLiteral("embossed"));
    m_regConfirm->setPlaceholderText(tr("Confirmer le mot de passe"));
    m_regConfirm->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_regName);
    layout->addWidget(m_regEmail);
    layout->addWidget(m_regPassword);
    layout->addWidget(m_regConfirm);

    auto *create = new QPushButton(tr("CRÉER LE COMPTE"), page);
    create->setObjectName(QStringLiteral("embossButton"));
    layout->addWidget(create);
    layout->addStretch(1);

    connect(create, &QPushButton::clicked, this, &LoginDialog::tryRegister);
    return page;
}

void LoginDialog::switchMode(int index)
{
    m_stack->setCurrentIndex(index);
    m_tabLogin->setChecked(index == 0);
    m_tabRegister->setChecked(index == 1);
}

void LoginDialog::applyAccent(const QColor &accent)
{
    m_accent = accent;
    m_card->setStyleSheet(cardStyle(accent));
}

void LoginDialog::animateGlowTo(const QColor &color)
{
    auto *animation = new QVariantAnimation(this);
    animation->setStartValue(m_lamp->glow());
    animation->setEndValue(color);
    animation->setDuration(500);
    connect(animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_lamp->setGlow(value.value<QColor>());
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void LoginDialog::onPull()
{
    ++m_colorIndex;
    if (m_colorIndex >= kLightColors.size()) {
        m_colorIndex = -1;
        m_lamp->setLit(false);
        m_card->setEnabled(false);
        auto *fade = new QPropertyAnimation(m_cardEffect, "opacity", this);
        fade->setDuration(400);
        fade->setStartValue(m_cardEffect->opacity());
        fade->setEndValue(0.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
        return;
    }

    const QColor color = kLightColors.at(m_colorIndex);
    applyAccent(color);
    if (!m_lamp->isLit()) {
        m_lamp->setGlow(color);
        m_lamp->setLit(true);
        m_card->setEnabled(true);
        auto *fade = new QPropertyAnimation(m_cardEffect, "opacity", this);
        fade->setDuration(600);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        animateGlowTo(color);
    }
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
        switchMode(0);
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
