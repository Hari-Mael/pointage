#pragma once

#include <QDialog>

#include "database.h"
#include "supabaseauth.h"

class QCheckBox;
class QLineEdit;
class QStackedWidget;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    LoginDialog(Database &db, const QString &dataDir, QWidget *parent = nullptr);
    UserRecord user() const { return m_user; }
    bool restoreSession();

private slots:
    void tryLogin();
    void tryRegister();
    void forgotPassword();

private:
    bool acceptSession(const SupabaseSession &session, bool remember);
    bool mirrorUser(const SupabaseSession &session);

    Database &m_db;
    QString m_dataDir;
    SupabaseAuth m_auth;
    UserRecord m_user;
    QStackedWidget *m_stack = nullptr;
    QLineEdit *m_loginEmail = nullptr;
    QLineEdit *m_loginPassword = nullptr;
    QCheckBox *m_rememberMe = nullptr;
    QLineEdit *m_regName = nullptr;
    QLineEdit *m_regEmail = nullptr;
    QLineEdit *m_regPassword = nullptr;
    QLineEdit *m_regConfirm = nullptr;
};
