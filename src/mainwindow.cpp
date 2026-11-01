#include "mainwindow.h"

#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "fields.h"
#include "persondialog.h"
#include "presenceprinter.h"
#include "restdaysdialog.h"

namespace {

constexpr int kColId = 0;
constexpr int kColName = 1;
constexpr int kColFirstDay = 2;
constexpr int kDayColumnWidth = 34;

QTableWidgetItem *makeItem(const QString &text, Qt::Alignment alignment = Qt::AlignCenter)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsEnabled);
    item->setTextAlignment(alignment | Qt::AlignVCenter);
    return item;
}

PresenceSheet buildSheet(const Database &db, const QString &title, const std::vector<PersonRecord> &people,
                         int year, int month, int restMask)
{
    PresenceSheet sheet;
    sheet.title = title;
    sheet.year = year;
    sheet.month = month;
    sheet.restMask = restMask;
    for (const PersonRecord &person : people)
        sheet.rows.push_back({person.code, person.name, db.marks(person.id, year, month)});
    return sheet;
}

}

MainWindow::MainWindow(Database &db, const UserRecord &user, QWidget *parent)
    : QMainWindow(parent), m_db(db), m_user(user)
{
    setWindowTitle(tr("Pointage — %1").arg(user.name));
    resize(1280, 760);
    buildUi();
    refresh();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    const QLocale french(QLocale::French);

    m_kindBox = new QComboBox(this);
    for (const KindDef &def : allKinds())
        m_kindBox->addItem(def.plural);

    m_monthBox = new QComboBox(this);
    for (int month = 1; month <= 12; ++month)
        m_monthBox->addItem(french.monthName(month));

    m_yearBox = new QSpinBox(this);
    m_yearBox->setRange(2000, 2100);

    const QDate today = QDate::currentDate();
    m_monthBox->setCurrentIndex(today.month() - 1);
    m_yearBox->setValue(today.year());

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Rechercher…"));

    auto *addButton = new QPushButton(tr("Ajouter"), this);
    auto *restButton = new QPushButton(tr("Jours de repos"), this);
    auto *logoutButton = new QPushButton(tr("Déconnexion"), this);

    auto *top = new QHBoxLayout;
    top->addWidget(m_kindBox);
    top->addWidget(m_monthBox);
    top->addWidget(m_yearBox);
    top->addWidget(m_search, 1);
    top->addWidget(addButton);
    top->addWidget(restButton);
    top->addWidget(logoutButton);
    layout->addLayout(top);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);
    layout->addWidget(m_table, 1);

    m_summary = new QLabel(this);
    layout->addWidget(m_summary);

    setCentralWidget(central);

    connect(m_kindBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refresh(); });
    connect(m_monthBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refresh(); });
    connect(m_yearBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { refresh(); });
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applySearch);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addPerson);
    connect(restButton, &QPushButton::clicked, this, &MainWindow::editRestDays);
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    connect(m_table, &QTableWidget::cellClicked, this, &MainWindow::onCellClicked);
}

void MainWindow::refresh()
{
    const KindDef &def = kindDef(kind());
    const int year = selectedYear();
    const int month = selectedMonth();
    const int days = QDate(year, month, 1).daysInMonth();
    const int presentCol = kColFirstDay + days;
    const int absentCol = presentCol + 1;
    const int actionsCol = absentCol + 1;
    const QLocale french(QLocale::French);

    m_people = m_db.persons(m_user.id, kind());

    m_table->setRowCount(0);
    m_table->setColumnCount(actionsCol + 1);

    QStringList headers{tr("ID"), tr("Nom")};
    for (int day = 1; day <= days; ++day) {
        const int weekday = QDate(year, month, day).dayOfWeek();
        headers << QStringLiteral("%1\n%2").arg(french.dayName(weekday, QLocale::ShortFormat).left(2)).arg(day);
    }
    headers << tr("P") << tr("A") << tr("Actions");
    m_table->setHorizontalHeaderLabels(headers);

    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(actionsCol, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColId, QHeaderView::ResizeToContents);
    for (int col = kColFirstDay; col < presentCol; ++col)
        m_table->setColumnWidth(col, kDayColumnWidth);

    const int rowCount = static_cast<int>(m_people.size());
    m_table->setRowCount(rowCount);

    int totalPresent = 0;
    int totalAbsent = 0;
    for (int row = 0; row < rowCount; ++row) {
        const PersonRecord &person = m_people[static_cast<size_t>(row)];
        const MarkMap marks = m_db.marks(person.id, year, month);
        int present = 0;
        int absent = 0;

        QTableWidgetItem *idItem = makeItem(person.code, Qt::AlignLeft);
        idItem->setData(Qt::UserRole, person.id);
        m_table->setItem(row, kColId, idItem);
        m_table->setItem(row, kColName, makeItem(person.name, Qt::AlignLeft));

        for (int day = 1; day <= days; ++day) {
            QTableWidgetItem *cell = makeItem(QString());
            const QDate date(year, month, day);
            const QChar status = marks.value(day);
            if (isRestDay(date)) {
                cell->setBackground(QColor("#e6eaf0"));
            } else if (status == QChar('P')) {
                ++present;
                cell->setText(QStringLiteral("P"));
                cell->setForeground(QColor("#1b8a4b"));
                cell->setBackground(QColor("#dff5e6"));
            } else if (status == QChar('A')) {
                ++absent;
                cell->setText(QStringLiteral("A"));
                cell->setForeground(QColor("#d12f3a"));
                cell->setBackground(QColor("#fde2e4"));
            }
            m_table->setItem(row, kColFirstDay + day - 1, cell);
        }

        m_table->setItem(row, presentCol, makeItem(QString::number(present)));
        m_table->setItem(row, absentCol, makeItem(QString::number(absent)));
        m_table->setCellWidget(row, actionsCol, makeActionWidget(person.id));

        totalPresent += present;
        totalAbsent += absent;
    }

    m_summary->setText(tr("Présences : %1   Absences : %2   %3 : %4")
                           .arg(totalPresent)
                           .arg(totalAbsent)
                           .arg(def.plural)
                           .arg(rowCount));
    applySearch();
}

void MainWindow::applySearch()
{
    const QString needle = m_search->text().trimmed();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QString code = m_table->item(row, kColId)->text();
        const QString name = m_table->item(row, kColName)->text();
        const bool match = needle.isEmpty()
                           || code.contains(needle, Qt::CaseInsensitive)
                           || name.contains(needle, Qt::CaseInsensitive);
        m_table->setRowHidden(row, !match);
    }
}

void MainWindow::onCellClicked(int row, int column)
{
    const int days = QDate(selectedYear(), selectedMonth(), 1).daysInMonth();
    if (column < kColFirstDay || column >= kColFirstDay + days)
        return;

    const QDate date(selectedYear(), selectedMonth(), column - kColFirstDay + 1);
    if (isRestDay(date))
        return;

    const QTableWidgetItem *idItem = m_table->item(row, kColId);
    const QTableWidgetItem *cell = m_table->item(row, column);
    if (!idItem || !cell)
        return;

    const int personId = idItem->data(Qt::UserRole).toInt();
    const QString current = cell->text();

    QChar next;
    if (current.isEmpty())
        next = QChar('P');
    else if (current == QLatin1String("P"))
        next = QChar('A');

    m_db.setMark(personId, date, next);
    refresh();
}

void MainWindow::addPerson()
{
    const KindDef &def = kindDef(kind());
    QStringList codes;
    for (const PersonRecord &person : m_people)
        codes << person.code;

    PersonDialog dialog(kind(), m_db.nextCode(m_user.id, kind(), def.prefix), codes, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!m_db.addPerson(m_user.id, kind(), dialog.person()))
        QMessageBox::critical(this, windowTitle(), tr("Impossible d'enregistrer la fiche (identifiant déjà utilisé ?)."));
    refresh();
}

void MainWindow::editRestDays()
{
    RestDaysDialog dialog(m_user.restMask, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_user.restMask = dialog.mask();
    m_db.setRestMask(m_user.id, m_user.restMask);
    refresh();
}

void MainWindow::printPerson(int personId)
{
    for (const PersonRecord &person : m_people) {
        if (person.id != personId)
            continue;
        const PresenceSheet sheet = buildSheet(m_db, tr("Fiche de présence — %1").arg(person.name),
                                               {person}, selectedYear(), selectedMonth(), m_user.restMask);
        printPresenceSheet(this, sheet);
        return;
    }
}

void MainWindow::printAll()
{
    const PresenceSheet sheet = buildSheet(m_db, tr("Liste de présence — %1").arg(kindDef(kind()).plural),
                                           m_people, selectedYear(), selectedMonth(), m_user.restMask);
    printPresenceSheet(this, sheet);
}

void MainWindow::deletePerson(int personId)
{
    for (const PersonRecord &person : m_people) {
        if (person.id != personId)
            continue;
        const auto answer = QMessageBox::question(this, windowTitle(),
                                                  tr("Supprimer %1 et tout son historique ?").arg(person.name));
        if (answer != QMessageBox::Yes)
            return;
        m_db.deletePerson(personId);
        refresh();
        return;
    }
}

QWidget *MainWindow::makeActionWidget(int personId)
{
    auto *box = new QWidget;
    auto *layout = new QHBoxLayout(box);
    layout->setContentsMargins(2, 2, 2, 2);

    auto *printButton = new QPushButton(tr("Imprimer"), box);
    auto *listButton = new QPushButton(tr("Liste de présence"), box);
    auto *deleteButton = new QPushButton(tr("Supprimer"), box);
    deleteButton->setStyleSheet(QStringLiteral("color:#d12f3a;"));

    layout->addWidget(printButton);
    layout->addWidget(listButton);
    layout->addWidget(deleteButton);

    connect(printButton, &QPushButton::clicked, this, [this, personId]() { printPerson(personId); });
    connect(listButton, &QPushButton::clicked, this, &MainWindow::printAll);
    connect(deleteButton, &QPushButton::clicked, this, [this, personId]() { deletePerson(personId); });
    return box;
}
