#pragma once

#include <QComboBox>
#include <QDate>
#include <QHash>
#include <QMainWindow>
#include <QPointer>
#include <QSpinBox>
#include <array>
#include <vector>

#include "cloudstore.h"
#include "dashboard.h"
#include "reports.h"

class DashboardDialog;
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
    void openClassBoard();
    void openContractBoard();
    void openStudentReport(const QString &personId);
    void openDashboard();
    void liveUpdate();
    void changeUserAvatar();
    void toggleSidebar();

private:
    void buildUi();
    QWidget *buildHeader();
    QWidget *buildDashboardPage();
    QWidget *buildActionsPage();
    QWidget *buildAnalysisPage();
    QWidget *buildPlansPage();
    QWidget *buildSettingsPage();
    QWidget *buildRestBar();
    QWidget *buildTableCard();
    QWidget *buildStats();
    QWidget *makeActionWidget(const QString &personId);
    const CloudPerson *findPerson(const QString &personId) const;
    void reportWriteResult(bool ok, const QString &action);
    void saveReport(const QString &defaultName, const QString &content);
    void refreshAvatar();
    Dashboard::Snapshot buildSnapshot() const;

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
    QPointer<DashboardDialog> m_dashboard;
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
    QPushButton *m_statPresent = nullptr;
    QPushButton *m_statAbsent = nullptr;
    QPushButton *m_statEmp = nullptr;
    QPushButton *m_statStu = nullptr;
    std::array<QPushButton *, 7> m_chips{};
};
