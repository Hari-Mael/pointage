#pragma once

#include <QColor>
#include <QString>

#include "cloudstore.h"

class QFrame;
class QLabel;
class QVBoxLayout;
class QWidget;

struct CardTone {
    QColor background;
    QColor foreground;
    QColor dot;
};

struct ColumnParts {
    QFrame *frame = nullptr;
    QLabel *count = nullptr;
    QVBoxLayout *layout = nullptr;
};

namespace PipelineCards {

ColumnParts makeColumn(const QString &title, const QColor &dot, QWidget *parent);
QWidget *personCard(const CloudPerson &person, const QString &subtitle, const CardTone &tone, QWidget *parent);

}
