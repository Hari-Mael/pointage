#pragma once

#include <QMap>
#include <QString>
#include <QWidget>
#include <vector>

struct PresenceRow {
    QString code;
    QString name;
    QMap<int, QChar> marks;
};

struct PresenceSheet {
    QString title;
    int year = 0;
    int month = 0;
    int restMask = 1;
    std::vector<PresenceRow> rows;
};

QString presenceHtml(const PresenceSheet &sheet);
void printPresenceSheet(QWidget *parent, const PresenceSheet &sheet);
