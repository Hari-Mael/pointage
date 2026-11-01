#pragma once

#include <QDialog>

#include "database.h"

class QLineEdit;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(Database &db, QWidget *parent = nullptr);
    UserRecord user() const { return m_user; }

private slots:
    void tryLogin();
    void tryRegister();

private:
    Database &m_db;
    UserRecord m_user;
    QLineEdit *m_loginEmail = nullptr;
    QLineEdit *m_loginPassword = nullptr;
    QLineEdit *m_regName = nullptr;
    QLineEdit *m_regEmail = nullptr;
    QLineEdit *m_regPassword = nullptr;
    QLineEdit *m_regConfirm = nullptr;
};
