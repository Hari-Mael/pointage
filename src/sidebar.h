#pragma once

#include <QFrame>
#include <vector>

class QPushButton;

class Sidebar : public QFrame {
    Q_OBJECT
public:
    enum Section { Home, Dashboard, Actions, Analysis, Plans, Settings };

    explicit Sidebar(QWidget *parent = nullptr);
    void setCurrentSection(Section section);

signals:
    void sectionChanged(int index);
    void logoutRequested();

private:
    std::vector<QPushButton *> m_items;
};
