#pragma once

#include <QString>
#include <QTime>

struct Schedule {
    QTime morningStart{6, 0};
    QTime morningEnd{12, 0};
    QTime pauseStart{12, 0};
    QTime pauseEnd{14, 0};
    QTime afternoonStart{14, 0};
    QTime afternoonEnd{18, 0};

    static Schedule load();
    void save() const;
    bool isValid() const;
    QString summary() const;
};
