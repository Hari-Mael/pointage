#pragma once

#include <QDialog>

#include "dashboard.h"

class QLabel;
class QVBoxLayout;

class DashboardDialog : public QDialog {
    Q_OBJECT
public:
    explicit DashboardDialog(QWidget *parent = nullptr);
    void setSnapshot(const Dashboard::Snapshot &snapshot);

private slots:
    void exportExcel();

private:
    QWidget *makeKpi(const QString &caption, QLabel *&valueLabel, QWidget *parent);
    void rebuildCharts();

    Dashboard::Snapshot m_snapshot;
    QLabel *m_title = nullptr;
    QLabel *m_updated = nullptr;
    QLabel *m_presence = nullptr;
    QLabel *m_absence = nullptr;
    QLabel *m_late = nullptr;
    QVBoxLayout *m_chartsLayout = nullptr;
    QWidget *m_chartsHost = nullptr;
};
