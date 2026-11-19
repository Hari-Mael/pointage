#include "sidebar.h"

#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

Sidebar::Sidebar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setFixedWidth(230);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 18, 14, 16);
    layout->setSpacing(4);

    auto *title = new QLabel(QStringLiteral("📋 Feuille de Pointage"), this);
    title->setObjectName(QStringLiteral("sidebarTitle"));
    layout->addWidget(title);

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);

    auto addHeader = [this, layout](const QString &text) {
        auto *header = new QLabel(text, this);
        header->setObjectName(QStringLiteral("navHeader"));
        layout->addWidget(header);
    };
    auto addItem = [this, layout, group](const QString &text, Section section) {
        auto *item = new QPushButton(text, this);
        item->setObjectName(QStringLiteral("navItem"));
        item->setCheckable(true);
        item->setCursor(Qt::PointingHandCursor);
        group->addButton(item);
        layout->addWidget(item);
        m_items.push_back(item);
        connect(item, &QPushButton::clicked, this, [this, section]() {
            setCurrentSection(section);
            emit sectionChanged(static_cast<int>(section));
        });
    };

    addHeader(tr("PRINCIPAL"));
    addItem(tr("📊  Tableau de bord"), Dashboard);
    addHeader(tr("GESTION"));
    addItem(tr("➕  Actions"), Actions);
    addItem(tr("📈  Analyse"), Analysis);
    addHeader(tr("CONFIGURATION"));
    addItem(tr("🗓  Plans"), Plans);
    addItem(tr("⚙  Paramètres"), Settings);

    layout->addStretch(1);

    auto *logout = new QPushButton(tr("🚪  Déconnexion"), this);
    logout->setProperty("variant", QStringLiteral("red"));
    connect(logout, &QPushButton::clicked, this, &Sidebar::logoutRequested);
    layout->addWidget(logout);

    setCurrentSection(Dashboard);
}

void Sidebar::setCurrentSection(Section section)
{
    const size_t index = static_cast<size_t>(section);
    for (size_t i = 0; i < m_items.size(); ++i)
        m_items[i]->setChecked(i == index);
}
