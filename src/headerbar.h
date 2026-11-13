#pragma once

#include <QComboBox>
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
    void setAvatar(const QPixmap &avatar);
    void setPendingCount(int count);
    void setSubtitle(const QString &text);

signals:
    void homeRequested();
    void dashboardRequested();
    void settingsRequested();
    void logoutRequested();

private:
    QPushButton *m_bell = nullptr;
    QPushButton *m_avatar = nullptr;
    QLabel *m_school = nullptr;
    QLabel *m_subtitle = nullptr;
    QComboBox *m_month = nullptr;
    QSpinBox *m_year = nullptr;
};
