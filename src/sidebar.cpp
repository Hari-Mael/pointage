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
    setStyleSheet(QStringLiteral(
        "QFrame#sidebar {"
        " background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "   stop:0 rgba(255,255,255,0.20), stop:0.45 rgba(255,255,255,0.07), stop:1 rgba(255,255,255,0.14));"
        " border: 1px solid rgba(255,255,255,0.30);"
        " border-radius: 28px;"
        "}"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 20, 14, 16);
    layout->setSpacing(4);

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
    addItem(tr("🏠  Accueil"), Home);
    addItem(tr("📊  Tableau de bord"), Dashboard);
    addHeader(tr("GESTION"));
    addItem(tr("➕  Actions"), Actions);
    addItem(tr("📋  Fiche de présence"), Analysis);
    addHeader(tr("CONFIGURATION"));
    addItem(tr("🗓  Plans"), Plans);
    addItem(tr("⚙  Paramètres"), Settings);

    layout->addStretch(1);

    auto *logout = new QPushButton(tr("🚪  Déconnexion"), this);
    logout->setProperty("variant", QStringLiteral("red"));
    connect(logout, &QPushButton::clicked, this, &Sidebar::logoutRequested);
    layout->addWidget(logout);

    setCurrentSection(Home);
}

void Sidebar::setCurrentSection(Section section)
{
    const size_t index = static_cast<size_t>(section);
    for (size_t i = 0; i < m_items.size(); ++i)
        m_items[i]->setChecked(i == index);
}
