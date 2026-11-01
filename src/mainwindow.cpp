#include "mainwindow.h"

#include <QComboBox>
#include <QFrame>
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
#include "presencedialog.h"

namespace {

constexpr int kColId = 0;
constexpr int kColName = 1;
constexpr int kColFirstDay = 2;
constexpr int kDayColumnWidth = 36;
constexpr int kActionsColumnWidth = 250;
constexpr int kRowHeight = 40;

const char *const kDayShort[7] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

QTableWidgetItem *makeItem(const QString &text, Qt::Alignment alignment = Qt::AlignCenter)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsEnabled);
    item->setTextAlignment(alignment | Qt::AlignVCenter);
    return item;
}

QFrame *makeCard(QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("card"));
    return card;
}

}

MainWindow::MainWindow(Database &db, const UserRecord &user, QWidget *parent)
    : QMainWindow(parent), m_db(db), m_user(user)
{
    setWindowTitle(tr("Feuille de Pointage Pro — %1").arg(user.name));
    resize(1180, 820);
    buildUi();
    refresh();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("page"));
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    root->addWidget(buildHeader());
    root->addWidget(buildRestBar());
    root->addWidget(buildSearchBar());
    root->addWidget(buildTableCard(), 1);

    setCentralWidget(central);
}

QWidget *MainWindow::buildHeader()
{
    QFrame *card = makeCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("📋 Feuille de Pointage Pro"), card);
    title->setObjectName(QStringLiteral("title"));
    auto *subtitle = new QLabel(QStringLiteral("%1 · %2").arg(m_user.name, m_user.email), card);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    layout->addWidget(title);
    layout->addWidget(subtitle);

    const QLocale french(QLocale::French);
    m_monthBox = new QComboBox(card);
    for (int month = 1; month <= 12; ++month)
        m_monthBox->addItem(french.monthName(month));
    m_yearBox = new QSpinBox(card);
    m_yearBox->setRange(2000, 2100);

    const QDate today = QDate::currentDate();
    m_monthBox->setCurrentIndex(today.month() - 1);
    m_yearBox->setValue(today.year());

    auto *logout = new QPushButton(tr("Déconnexion"), card);
    logout->setProperty("variant", QStringLiteral("ghost"));

    auto *controls = new QHBoxLayout;
    auto *monthLabel = new QLabel(tr("Mois :"), card);
    monthLabel->setObjectName(QStringLiteral("muted"));
    controls->addWidget(monthLabel);
    controls->addWidget(m_monthBox);
    controls->addWidget(m_yearBox);
    controls->addStretch(1);
    controls->addWidget(logout);
    layout->addLayout(controls);

    connect(m_monthBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refresh(); });
    connect(m_yearBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { refresh(); });
    connect(logout, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    return card;
}

QWidget *MainWindow::buildRestBar()
{
    auto *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("restBar"));
    auto *layout = new QVBoxLayout(bar);
    layout->setContentsMargins(16, 12, 16, 14);
    layout->setSpacing(10);

    auto *days = new QHBoxLayout;
    auto *label = new QLabel(tr("Jours de repos :"), bar);
    label->setObjectName(QStringLiteral("restLabel"));
    days->addWidget(label);
    for (int dow = 0; dow < 7; ++dow) {
        auto *chip = new QPushButton(QString::fromLatin1(kDayShort[dow]), bar);
        chip->setCheckable(true);
        chip->setProperty("variant", QStringLiteral("chip"));
        m_chips[static_cast<size_t>(dow)] = chip;
        connect(chip, &QPushButton::clicked, this, [this, dow]() { toggleRestDay(dow); });
        days->addWidget(chip);
    }
    days->addStretch(1);
    layout->addLayout(days);

    auto *actions = new QHBoxLayout;
    auto *addEmployee = new QPushButton(tr("Ajouter employé"), bar);
    addEmployee->setProperty("variant", QStringLiteral("teal"));
    auto *addStudent = new QPushButton(tr("Ajouter élève"), bar);
    actions->addWidget(addEmployee);
    actions->addWidget(addStudent);
    actions->addStretch(1);
    layout->addLayout(actions);

    connect(addEmployee, &QPushButton::clicked, this, [this]() { setKind(kKindEmployee); addPerson(); });
    connect(addStudent, &QPushButton::clicked, this, [this]() { setKind(kKindStudent); addPerson(); });
    return bar;
}

QWidget *MainWindow::buildSearchBar()
{
    QFrame *card = makeCard(this);
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    m_search = new QLineEdit(card);
    m_search->setPlaceholderText(tr("🔍 Rechercher par ID ou Nom..."));
    auto *clear = new QPushButton(tr("Effacer"), card);
    clear->setProperty("variant", QStringLiteral("ghost"));
    layout->addWidget(m_search, 1);
    layout->addWidget(clear);
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applySearch);
    connect(clear, &QPushButton::clicked, m_search, &QLineEdit::clear);
    return card;
}

QWidget *MainWindow::buildTableCard()
{
    QFrame *card = makeCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto *tabs = new QHBoxLayout;
    m_tabEmp = new QPushButton(card);
    m_tabEmp->setCheckable(true);
    m_tabEmp->setProperty("variant", QStringLiteral("tab"));
    m_tabStu = new QPushButton(card);
    m_tabStu->setCheckable(true);
    m_tabStu->setProperty("variant", QStringLiteral("tab"));
    tabs->addWidget(m_tabEmp);
    tabs->addWidget(m_tabStu);
    tabs->addStretch(1);
    layout->addLayout(tabs);
    connect(m_tabEmp, &QPushButton::clicked, this, [this]() { setKind(kKindEmployee); });
    connect(m_tabStu, &QPushButton::clicked, this, [this]() { setKind(kKindStudent); });

    m_table = new QTableWidget(card);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    connect(m_table, &QTableWidget::cellClicked, this, &MainWindow::onCellClicked);
    layout->addWidget(m_table, 1);

    layout->addWidget(buildStats());

    auto *hint = new QLabel(tr("Cliquez sur une case : vide → P → A → vide"), card);
    hint->setObjectName(QStringLiteral("muted"));
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
    return card;
}

QWidget *MainWindow::buildStats()
{
    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_statPresent = new QPushButton(row);
    m_statPresent->setProperty("variant", QStringLiteral("teal"));
    m_statPresent->setProperty("stat", QStringLiteral("true"));
    m_statPresent->setFocusPolicy(Qt::NoFocus);

    m_statAbsent = new QPushButton(row);
    m_statAbsent->setProperty("variant", QStringLiteral("red"));
    m_statAbsent->setProperty("stat", QStringLiteral("true"));
    m_statAbsent->setFocusPolicy(Qt::NoFocus);

    m_statEmp = new QPushButton(row);
    m_statEmp->setProperty("stat", QStringLiteral("true"));
    connect(m_statEmp, &QPushButton::clicked, this, [this]() { setKind(kKindEmployee); });

    m_statStu = new QPushButton(row);
    m_statStu->setProperty("stat", QStringLiteral("true"));
    connect(m_statStu, &QPushButton::clicked, this, [this]() { setKind(kKindStudent); });

    layout->addWidget(m_statPresent, 1);
    layout->addWidget(m_statAbsent, 1);
    layout->addWidget(m_statEmp, 1);
    layout->addWidget(m_statStu, 1);
    return row;
}

void MainWindow::refresh()
{
    const int year = selectedYear();
    const int month = selectedMonth();
    const int days = QDate(year, month, 1).daysInMonth();
    const int presentCol = kColFirstDay + days;
    const int absentCol = presentCol + 1;
    const int actionsCol = absentCol + 1;
    const QLocale french(QLocale::French);

    const size_t employees = m_db.persons(m_user.id, kKindEmployee).size();
    const size_t students = m_db.persons(m_user.id, kKindStudent).size();
    m_people = m_db.persons(m_user.id, m_kind);

    m_tabEmp->setText(tr("Employés (%1)").arg(employees));
    m_tabStu->setText(tr("Élèves (%1)").arg(students));
    m_tabEmp->setChecked(m_kind == kKindEmployee);
    m_tabStu->setChecked(m_kind == kKindStudent);
    for (int dow = 0; dow < 7; ++dow)
        m_chips[static_cast<size_t>(dow)]->setChecked((m_user.restMask >> dow) & 1);

    m_table->setRowCount(0);
    m_table->setColumnCount(actionsCol + 1);

    QStringList headers{tr("ID"), tr("Nom")};
    for (int day = 1; day <= days; ++day) {
        const int dow = QDate(year, month, day).dayOfWeek() % 7;
        headers << QStringLiteral("%1\n%2").arg(QString::fromLatin1(kDayShort[dow])).arg(day);
    }
    headers << tr("P") << tr("A") << tr("Actions");
    m_table->setHorizontalHeaderLabels(headers);

    m_table->horizontalHeader()->setSectionResizeMode(kColId, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(actionsCol, QHeaderView::Fixed);
    m_table->setColumnWidth(actionsCol, kActionsColumnWidth);
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
                cell->setBackground(QColor("#dfe4ec"));
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

    m_statPresent->setText(tr("Présences : %1").arg(totalPresent));
    m_statAbsent->setText(tr("Absences : %1").arg(totalAbsent));
    m_statEmp->setText(tr("Employés : %1").arg(employees));
    m_statStu->setText(tr("Élèves : %1").arg(students));
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

QWidget *MainWindow::makeActionWidget(int personId)
{
    auto *box = new QWidget;
    auto *layout = new QHBoxLayout(box);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(6);

    auto *insertButton = new QPushButton(tr("Insérer"), box);
    insertButton->setProperty("variant", QStringLiteral("teal"));
    insertButton->setProperty("small", QStringLiteral("true"));
    auto *modifyButton = new QPushButton(tr("Modifier"), box);
    modifyButton->setProperty("small", QStringLiteral("true"));
    auto *deleteButton = new QPushButton(tr("Supprimer"), box);
    deleteButton->setProperty("variant", QStringLiteral("red"));
    deleteButton->setProperty("small", QStringLiteral("true"));

    layout->addWidget(insertButton);
    layout->addWidget(modifyButton);
    layout->addWidget(deleteButton);

    connect(insertButton, &QPushButton::clicked, this, [this, personId]() { insertPresence(personId); });
    connect(modifyButton, &QPushButton::clicked, this, [this, personId]() { modifyPresence(personId); });
    connect(deleteButton, &QPushButton::clicked, this, [this, personId]() { deletePerson(personId); });
    return box;
}

const PersonRecord *MainWindow::findPerson(int personId) const
{
    for (const PersonRecord &person : m_people) {
        if (person.id == personId)
            return &person;
    }
    return nullptr;
}

void MainWindow::setKind(int kind)
{
    m_kind = kind;
    refresh();
}

void MainWindow::toggleRestDay(int dayOfWeek)
{
    m_user.restMask ^= (1 << dayOfWeek);
    m_db.setRestMask(m_user.id, m_user.restMask);
    refresh();
}

void MainWindow::addPerson()
{
    const KindDef &def = kindDef(m_kind);
    QStringList codes;
    for (const PersonRecord &person : m_people)
        codes << person.code;

    PersonDialog dialog(m_kind, m_db.nextCode(m_user.id, m_kind, def.prefix), codes, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!m_db.addPerson(m_user.id, m_kind, dialog.person()))
        QMessageBox::critical(this, windowTitle(), tr("Impossible d'enregistrer la fiche (identifiant déjà utilisé ?)."));
    refresh();
}

void MainWindow::insertPresence(int personId)
{
    const PersonRecord *person = findPerson(personId);
    if (!person)
        return;

    const int year = selectedYear();
    const int month = selectedMonth();
    const MarkMap existing = m_db.marks(personId, year, month);

    PresenceDialog dialog(PresenceDialog::Mode::Insert, person->name, year, month, existing, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    int added = 0;
    int skipped = 0;
    for (const QDate &date : dialog.dates()) {
        if (isRestDay(date) || existing.contains(date.day())) {
            ++skipped;
            continue;
        }
        m_db.setMark(personId, date, dialog.status());
        ++added;
    }
    refresh();
    QMessageBox::information(this, windowTitle(),
                             tr("%1 case(s) ajoutée(s), %2 ignorée(s) (repos ou déjà pointées).").arg(added).arg(skipped));
}

void MainWindow::modifyPresence(int personId)
{
    const PersonRecord *person = findPerson(personId);
    if (!person)
        return;

    const int year = selectedYear();
    const int month = selectedMonth();
    const MarkMap marks = m_db.marks(personId, year, month);
    if (marks.isEmpty()) {
        QMessageBox::information(this, windowTitle(),
                                 tr("Aucune case pointée ce mois-ci pour %1.").arg(person->name));
        return;
    }

    PresenceDialog dialog(PresenceDialog::Mode::Modify, person->name, year, month, marks, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QDate date = dialog.dates().first();
    m_db.setMark(personId, date, dialog.status());
    refresh();
}

void MainWindow::deletePerson(int personId)
{
    const PersonRecord *person = findPerson(personId);
    if (!person)
        return;

    const auto answer = QMessageBox::question(this, windowTitle(),
                                              tr("Supprimer %1 et tout son historique ?").arg(person->name));
    if (answer != QMessageBox::Yes)
        return;

    m_db.deletePerson(personId);
    refresh();
}
