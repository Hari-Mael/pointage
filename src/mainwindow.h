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
class QLabel;
class QTimer;
class QLineEdit;
class QPushButton;
class QTableWidget;

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

private:
    void buildUi();
    QWidget *buildHeader();
    QWidget *buildRestBar();
    QWidget *buildSearchBar();
    QWidget *buildTableCard();
    QWidget *buildStats();
    QWidget *makeActionWidget(const QString &personId);
    const CloudPerson *findPerson(const QString &personId) const;
    void reportWriteResult(bool ok, const QString
