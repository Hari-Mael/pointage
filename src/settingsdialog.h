#pragma once

#include <QDialog>

#include "schedule.h"

class QTimeEdit;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void scheduleSaved(const Schedule &schedule);

private slots:
    void changeLogo();
    void resetLogo();
    void openPrivacy();
    void save();

private:
    void updateIcons();

    QTimeEdit *m_morningStart = nullptr;
    QTimeEdit *m_morningEnd = nullptr;
    QTimeEdit *m_pauseStart = nullptr;
    QTimeEdit *m_pauseEnd = nullptr;
    QTimeEdit *m_afternoonStart = nullptr;
    QTimeEdit *m_afternoonEnd = nullptr;
};
