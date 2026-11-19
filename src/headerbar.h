#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

class QLabel;

class HeaderBar : public QWidget {
    Q_OBJECT
public:
    HeaderBar(const QString &userName, const QString &userEmail, const QPixmap &avatar, QWidget *parent = nullptr);

    QComboBox *monthBox() const { return m_month; }
    QSpinBox *yearBox() const { return m_year; }
    QPushButton *avatarButton() const { return m_avatar; }
    QLineEdit *searchEdit() const { return m_search; }
    void setAvatar(const QPixmap &avatar);
    void setPendingCount(int count);

signals:
    void homeRequested();
    void settingsRequested();
    void menuToggleRequested();

private:
    QPushButton *m_menuToggle = nullptr;
    QPushButton *m_bell = nullptr;
    QPushButton *m_avatar = nullptr;
    QLabel *m_school = nullptr;
    QLineEdit *m_search = nullptr;
    QComboBox *m_month = nullptr;
    QSpinBox *m_year = nullptr;
};
