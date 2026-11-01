#pragma once

#include <QDialog>
#include <utility>
#include <vector>

class QCheckBox;

class RestDaysDialog : public QDialog {
    Q_OBJECT
public:
    explicit RestDaysDialog(int mask, QWidget *parent = nullptr);
    int mask() const;

private:
    std::vector<std::pair<int, QCheckBox *>> m_days;
};
