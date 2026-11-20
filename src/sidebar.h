#pragma once

#include <QFrame>
#include <array>

class QPushButton;

class Sidebar : public QFrame {
    Q_OBJECT
public:
    enum Section { Home, Dashboard, Actions, Analysis, Plans, Settings, SectionCount };

    explicit Sidebar(QWidget *parent = nullptr);
    void setCurrentSection(Section section);

signals:
    void sectionChanged(int index);
    void logoutRequested();

private:
    std::array<QPushButton *, SectionCount> m_items{};
};
