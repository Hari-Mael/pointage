#pragma once

#include <QComboBox>
#include <QDate>
#include <QMainWindow>
#include <QSpinBox>
#include <array>
#include <vector>

#include "database.h"

class QLineEdit;
class QPushButton;
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
    void insertPresence(int personId);
    void modifyPresence(int personId);
    void deletePerson(int personId);
    void toggleRestDay(int dayOfWeek);
    void setKind(int kind);

private:
    void buildUi();
    QWidget *buildHeader();
    QWidget *buildRestBar();
    QWidget *buildSearchBar();
    QWidget *buildTableCard();
    QWidget *buildStats();
    QWidget *makeActionWidget(int personId);
    const PersonRecord *findPerson(int personId) const;

    int selectedYear() const { return m_yearBox->value(); }
    int selectedMonth() const { return m_monthBox->currentIndex() + 1; }
    bool isRestDay(const QDate &date) const { return (m_user.restMask >> (date.dayOfWeek() % 7)) & 1; }

    Database &m_db;
    UserRecord m_user;
    int m_kind = 0;
    std::vector<PersonRecord> m_people;
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
