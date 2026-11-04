#pragma once

#include <QDialog>

#include "supabaseauth.h"

class QCheckBox;
class QLineEdit;
class QStackedWidget;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(const QString &dataDir, QWidget *parent = nullptr);
    SupabaseSession session() const { return m_session; }
    bool restoreSession();

private slots:
    void tryLogin();
    void tryRegister();
    void forgotPassword();

private:
    bool acceptSession(const SupabaseSession &session, bool remember);

    QString m_dataDir;
    SupabaseAuth m_auth;
    SupabaseSession m_session;
    QStackedWidget *m_stack = nullptr;
    QLineEdit *m_loginEmail = nullptr;
    QLineEdit *m_loginPassword = nullptr;
    QCheckBox *m_rememberMe = nullptr;
    QLineEdit *m_regName = nullptr;
    QLineEdit *m_regEmail = nullptr;
    QLineEdit *m_regPassword = nullptr;
    QLineEdit *m_regConfirm = nullptr;
};
