#pragma once

#include <QChar>
#include <QDate>
#include <QDialog>
#include <QList>

#include "database.h"

class QComboBox;
class QDateEdit;
class QPushButton;

class PresenceDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode { Insert, Modify };

    PresenceDialog(Mode mode, const QString &personName, int year, int month, const MarkMap &marks,
                   QWidget *parent = nullptr);

    QChar status() const { return m_status; }
    QList<QDate> dates() const;

private slots:
    void onSave();

private:
    void setChoice(QChar status);

    Mode m_mode;
    int m_year;
    int m_month;
    QChar m_status;
    bool m_chosen = false;
    QPushButton *m_present = nullptr;
    QPushButton *m_absent = nullptr;
    QPushButton *m_clear = nullptr;
    QDateEdit *m_from = nullptr;
    QDateEdit *m_to = nullptr;
    QComboBox *m_day = nullptr;
};
