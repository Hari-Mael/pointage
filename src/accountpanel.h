#pragma once

#include <QWidget>

class CloudStore;
class QLineEdit;

class AccountPanel : public QWidget {
    Q_OBJECT
public:
    AccountPanel(CloudStore &store, const QString &currentEmail, QWidget *parent = nullptr);

private slots:
    void save();

private:
    CloudStore &m_store;
    QLineEdit *m_email = nullptr;
    QLineEdit *m_password = nullptr;
    QLineEdit *m_confirm = nullptr;
};
