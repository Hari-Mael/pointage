#pragma once

#include <QComboBox>
#include <QDate>
#include <QHash>
#include <QMainWindow>
#include <QSpinBox>
#include <QStringList>
#include <array>
#include <vector>

#include "cloudstore.h"
#include "reports.h"

class HeaderBar;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTimer;
class Sidebar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(CloudStore &store, const QString &userName, const QString &userEmail, QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void refresh();
    void applySearch();
    void onSearchChanged();
    void onCellClicked(int row, int column);
    void addPerson();
    void editPerson(const QString &personId);
    void insertPresence(const QString &personId);
    void modifyPresence(const QString &personId);
    void deletePerson(const QString &personId);
    void toggleRestDay(int dayOfWeek);
    void setKind(int kind);
    void importRoster();
    void exportRoster();
    void exportMonthly(const QString &personId);
    void exportRate(const QString &personId, Reports::Rate rate);
    void openSettings();
    void openStudentReport(const QString &personId);
    void openKpiReport(int index);
    void showNotifications();
    void showSection(int page);
    void liveUpdate();
    void changeUserAvatar();
    void toggleSidebar();

private:
    void buildUi();
    QWidget *buildHeader();
    QWidget *buildHomePage();
    QWidget *buildDashboardPage();
    QWidget *buildActionsPage();
    QWidget *buildAnalysisPage();
    QWidget *buildPlansPage();
    QWidget *buildSettingsPage();
    QWidget *buildRestBar();
    QWidget *buildTableCard();
    QWidget *makeActionWidget(const QString &personId);
    const CloudPerson *findPerson(const QString &personId) const;
    void reportWriteResult(bool ok, const QString &action);
    void saveReport(const QString &defaultName, const QString &content);
    void refreshAvatar();
    void updateAnalysis(int year, int month);

    int selectedYear() const { return m_yearBox->value(); }
    int selectedMonth() const { return m_monthBox->currentIndex() + 1; }
    bool isRestDay(const QDate &date) const { return (m_restMask >> (date.dayOfWeek() % 7)) & 1; }

    CloudStore &m_store;
    QString m_userName;
    QString m_userEmail;
    int m_restMask = 1;
    int m_kind = 0;
    std::vector<CloudPerson> m_all;
    std::vector<CloudPerson> m_people;
    QHash<QString, CloudMarks> m_marks;
    QTimer *m_liveTimer = nullptr;
    QPushButton *m_avatarButton = nullptr;
    HeaderBar *m_header = nullptr;
    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_pages = nullptr;
    QLabel *m_pendingLabel = nullptr;
    QString m_userAvatar;
    QComboBox *m_monthBox = nullptr;
    QSpinBox *m_yearBox = nullptr;
    QLineEdit *m_search = nullptr;
    QTableWidget *m_table = nullptr;
    QPushButton *m_tabEmp = nullptr;
    QPushButton *m_tabStu = nullptr;
    std::array<QPushButton *, 7> m_chips{};
    std::array<QLabel *, 3> m_homeValue{};
    std::array<QLabel *, 4> m_kpiValue{};
    std::array<QLabel *, 4> m_kpiSub{};
    QWidget *m_rate = nullptr;
    QWidget *m_chart = nullptr;
    QLabel *m_notifyBody = nullptr;

    int m_presentTotal = 0;
    int m_absentTotal = 0;
    int m_lateTotal = 0;
    int m_possibleTotal = 0;
    int m_peopleCount = 0;
    QStringList m_presenceLines;
    QStringList m_absenceLines;
    QStringList m_lateLines;
    QStringList m_notifications;
};
