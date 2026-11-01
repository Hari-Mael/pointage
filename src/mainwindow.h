#pragma once

#include <QComboBox>
#include <QDate>
#include <QMainWindow>
#include <QSpinBox>
#include <vector>

#include "database.h"

class QLabel;
class QLineEdit;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Database &db, const UserRecord &user, QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void refresh();
    void applySearch();
    void onCellClicked(int row, int column);
    void addPerson();
    void editRestDays();
    void printPerson(int personId);
    void printAll();
    void deletePerson(int personId);

private:
    void buildUi();
    QWidget *makeActionWidget(int personId);
    int kind() const { return m_kindBox->currentIndex(); }
    int selectedYear() const { return m_yearBox->value(); }
    int selectedMonth() const { return m_monthBox->currentIndex() + 1; }
    bool isRestDay(const QDate &date) const { return (m_user.restMask >> (date.dayOfWeek() % 7)) & 1; }

    Database &m_db;
    UserRecord m_user;
    std::vector<PersonRecord> m_people;
    QComboBox *m_kindBox = nullptr;
    QComboBox *m_monthBox = nullptr;
    QSpinBox *m_yearBox = nullptr;
    QLineEdit *m_search = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_summary = nullptr;
};
