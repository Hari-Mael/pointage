#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

class QLabel;
class QResizeEvent;

class HeaderBar : public QWidget {
    Q_OBJECT
public:
    HeaderBar(const QString &userName, const QString &userEmail, const QPixmap &avatar, QWidget *parent = nullptr);

    QComboBox *monthBox() const { return m_month; }
    QSpinBox *yearBox() const { return m_year; }
    QPushButton *avatarButton() const { return m_avatar; }
    QPushButton *bellButton() const { return m_bell; }
    QLineEdit *searchEdit() const { return m_search; }
    void setAvatar(const QPixmap &avatar);
    void setBellCount(int notifications, int pending);

signals:
    void menuToggleRequested();
    void notificationsRequested();
    void settingsRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QPushButton *m_menuToggle = nullptr;
    QPushButton *m_bell = nullptr;
    QPushButton *m_avatar = nullptr;
    QLabel *m_schoolTitle = nullptr;
    QLabel *m_profileName = nullptr;
    QLabel *m_profileMail = nullptr;
    QLineEdit *m_search = nullptr;
    QComboBox *m_month = nullptr;
    QSpinBox *m_year = nullptr;
};
