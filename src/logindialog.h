#pragma once

#include <QColor>
#include <QDialog>

#include "supabaseauth.h"

class LampWidget;
class QCheckBox;
class QFrame;
class QGraphicsOpacityEffect;
class QLineEdit;
class QPushButton;
class QStackedWidget;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(const QString &dataDir, QWidget *parent = nullptr);
    bool restoreSession();
    SupabaseSession session() const { return m_session; }

private slots:
    void tryLogin();
    void tryRegister();
    void forgotPassword();

private:
    bool acceptSession(const SupabaseSession &session, bool remember);
    void switchMode(int index);
    void applyAccent(const QColor &accent);
    void animateGlowTo(const QColor &color);
    void lightUp();
    QWidget *buildLoginPage();
    QWidget *buildRegisterPage();

    QString m_dataDir;
    SupabaseAuth m_auth;
    SupabaseSession m_session;
    QColor m_accent;
    LampWidget *m_lamp = nullptr;
    QFrame *m_card = nullptr;
    QGraphicsOpacityEffect *m_cardEffect = nullptr;
    QPushButton *m_tabLogin = nullptr;
    QPushButton *m_tabRegister = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLineEdit *m_loginEmail = nullptr;
    QLineEdit *m_loginPassword = nullptr;
    QLineEdit *m_regName = nullptr;
    QLineEdit *m_regEmail = nullptr;
    QLineEdit *m_regPassword = nullptr;
    QLineEdit *m_regConfirm = nullptr;
    QCheckBox *m_rememberMe = nullptr;
};
