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
#include <QFileDialog>

#include "exporter.h"
#include "privacynotice.h"
#include "fields.h"
#include "persondialog.h"
#include "presencedialog.h"

namespace {

constexpr int kColCheck = 0;
constexpr int kColId = 1;
constexpr int kColName = 2;
constexpr int kColFirstDay = 3;
constexpr int kDayColumnWidth = 36;
constexpr int kActionsColumnWidth = 270;
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

QString nextCodeFor(const std::vector<CloudPerson> &people, int kind, QChar prefix)
{
    int highest = 0;
    for (const CloudPerson &person : people) {
        if (person.kind != kind || !person.code.startsWith(prefix))
            continue;
        bool ok = false;
        const int number = person.code.mid(1).toInt(&ok);
        if (ok && number > highest)
            highest = number;
    }
    return QStringLiteral("%1%2").arg(prefix).arg(highest + 1, 3, 10, QLatin1Char('0'));
}

}

MainWindow::MainWindow(CloudStore &store, const QString &userName, const QString &userEmail, QWidget *parent)
    : QMainWindow(parent), m_store(store), m_userName(userName), m_userEmail(userEmail)
{
    setWindowTitle(tr("Feuille de Pointage Pro — %1").arg(userName));
    resize(1180, 820);
    buildUi();
    refresh();
}

void MainWindow::reportWriteResult(bool ok, const QString &action)
{
    if (!ok)
        QMessageBox::warning(this, windowTitle(), tr("%1 impossible : %2").arg(action, m_store.lastError()));
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
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(12);

    auto *titles = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("📋 Feuille de Pointage Pro"), card);
    title->setObjectName(QStringLiteral("title"));
    auto *subtitle = new QLabel(QStringLiteral("%1 · %2").arg(m_userName, m_userEmail), card);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    titles->addWidget(title);
    titles->addWidget(subtitle);
    layout->addLayout(titles, 1);

    const QLocale french(QLocale::French);
    m_monthBox = new QComboBox(card);
    for (int month = 1; month <= 12; ++month)
        m_monthBox->addItem(french.monthName(month));
    m_yearBox = new QSpinBox(card);
    m_yearBox->setRange(2000, 2100);

    const QDate today = QDate::currentDate();
    m_monthBox->setCurrentIndex(today.month() - 1);
    m_yearBox->setValue(today.year());

    auto *monthLabel = new QLabel(tr("Mois :"), card);
    monthLabel->setObjectName(QStringLiteral("muted"));
    auto *logout = new QPushButton(tr("Déconnexion"), card);
    logout->setProperty("variant", QStringLiteral("ghost"));

    layout->addWidget(monthLabel);
    layout->addWidget(m_monthBox);
    layout->addWidget(m_yearBox);
    layout->addWidget(logout);   
    auto *exportButton = new QPushButton(tr("Exporter CSV"), card);
    exportButton->setProperty("variant", QStringLiteral("ghost"));
    auto *privacyButton = new QPushButton(tr("Confidentialité"), card);
    privacyButton->setProperty("variant", QStringLiteral("ghost"));
    layout->addWidget(exportButton);
    layout->addWidget(privacyButton);
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Exporter en CSV"),
            QStringLiteral("pointage_%1_%2.csv").arg(selectedYear()).arg(selectedMonth(), 2, 10, QLatin1Char('0')),
            tr("Fichiers CSV (*.csv)"));
        if (path.isEmpty())
            return;
        const QString csv = Exporter::buildCsv(m_all, m_marks, m_kind, selectedYear(), selectedMonth());
        QString error;
        if (Exporter::writeCsv(path, csv, error))
            QMessageBox::information(this, windowTitle(), tr("Export enregistré : %1").arg(path));
        else
            QMessageBox::warning(this, windowTitle(), tr("Export impossible : %1").arg(error));
    });
    connect(privacyButton, &QPushButton::clicked, this, [this]() { showPrivacyNotice(this); });

    connect(m_monthBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refresh(); });
    connect(m_yearBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { refresh(); });
    connect(logout, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    return card;
}

QWidget *MainWindow::buildRestBar()
{
    auto *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("restBar"));
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);

    auto *label = new QLabel(tr("Jours de repos :"), bar);
    label->setObjectName(QStringLiteral("restLabel"));
    layout->addWidget(label);
    for (int dow = 0; dow < 7; ++dow) {
        auto *chip = new QPushButton(QString::fromLatin1(kDayShort[dow]), bar);
        chip->setCheckable(true);
        chip->setProperty("variant", QStringLiteral("chip"));
        m_chips[static_cast<size_t>(dow)] = chip;
        connect(chip, &QPushButton::clicked, this, [this, dow]() { toggleRestDay(dow); });
        layout->addWidget(chip);
    }
    layout->addStretch(1);

    auto *addEmployee = new QPushButton(tr("Ajouter employé"), bar);
    addEmployee->setProperty("variant", QStringLiteral("teal"));
    auto *addStudent = new QPushButton(tr("Ajouter élève"), bar);
    layout->addWidget(addEmployee);
    layout->addWidget(addStudent);

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

    if (!m_store.fetchPeople(m_all) || !m_store.fetchMarks(year, month, m_marks)
        || !m_store.fetchRestMask(m_restMask)) {
        QMessageBox::warning(this, windowTitle(), tr("Lecture des données impossible : %1").arg(m_store.lastError()));
    }

    size_t employees = 0;
    size_t students = 0;
    m_people.clear();
    for (const CloudPerson &person : m_all) {
        if (person.kind == kKindEmployee)
            ++employees;
        else
            ++students;
        if (person.kind == m_kind)
            m_people.push_back(person);
    }

    m_tabEmp->setText(tr("Employés (%1)").arg(employees));
    m_tabStu->setText(tr("Élèves (%1)").arg(students));
    m_tabEmp->setChecked(m_kind == kKindEmployee);
    m_tabStu->setChecked(m_kind == kKindStudent);
    for (int dow = 0; dow < 7; ++dow)
        m_chips[static_cast<size_t>(dow)]->setChecked((m_restMask >> dow) & 1);

    m_table->setRowCount(0);
    m_table->setColumnCount(actionsCol + 1);

    QStringList headers{QString(), tr("ID"), tr("Nom")};
    for (int day = 1; day <= days; ++day) {
        const int dow = QDate(year, month, day).dayOfWeek() % 7;
        headers << QStringLiteral("%1\n%2").arg(QString::fromLatin1(kDayShort[dow])).arg(day);
    }
    headers << tr("P") << tr("A") << tr("Actions");
    m_table->setHorizontalHeaderLabels(headers);

    m_table->horizontalHeader()->setSectionResizeMode(kColCheck, QHeaderView::Fixed);
    m_table->setColumnWidth(kColCheck, 36);
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
        const CloudPerson &person = m_people[static_cast<size_t>(row)];
        const CloudMarks marks = m_marks.value(person.id);
        int present = 0;
        int absent = 0;

        auto *check = new QTableWidgetItem;
        check->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        check->setCheckState(Qt::Unchecked);
        m_table->setItem(row, kColCheck, check);

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

    const QString personId = idItem->data(Qt::UserRole).toString();
    const QString current = cell->text();

    QChar next;
    if (current.isEmpty())
        next = QChar('P');
    else if (current == QLatin1String("P"))
        next = QChar('A');

    reportWriteResult(m_store.setMark(personId, date, next), tr("Enregistrement"));
    refresh();
}

QWidget *MainWindow::makeActionWidget(const QString &personId)
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

const CloudPerson *MainWindow::findPerson(const QString &personId) const
{
    for (const CloudPerson &person : m_people) {
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
    const int mask = m_restMask ^ (1 << dayOfWeek);
    reportWriteResult(m_store.setRestMask(mask), tr("Enregistrement des jours de repos"));
    refresh();
}

void MainWindow::addPerson()
{
    const KindDef &def = kindDef(m_kind);
    QStringList codes;
    for (const CloudPerson &person : m_all) {
        if (person.kind == m_kind)
            codes << person.code;
    }

    PersonDialog dialog(m_kind, nextCodeFor(m_all, m_kind, def.prefix), codes, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const PersonRecord created = dialog.person();
    CloudPerson person;
    person.kind = m_kind;
    person.code = created.code;
    person.name = created.name;
    person.fields = created.fields;
    reportWriteResult(m_store.addPerson(person), tr("Ajout de la fiche"));
    refresh();
}

void MainWindow::insertPresence(const QString &personId)
{
    const CloudPerson *person = findPerson(personId);
    if (!person)
        return;

    const int year = selectedYear();
    const int month = selectedMonth();
    const CloudMarks existing = m_marks.value(personId);

    PresenceDialog dialog(PresenceDialog::Mode::Insert, person->name, year, month, existing, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    int added = 0;
    int skipped = 0;
    int failed = 0;
    for (const QDate &date : dialog.dates()) {
        if (isRestDay(date) || existing.contains(date.day())) {
            ++skipped;
            continue;
        }
        if (m_store.setMark(personId, date, dialog.status()))
            ++added;
        else
            ++failed;
    }
    refresh();
    if (failed > 0)
        QMessageBox::warning(this, windowTitle(), tr("%1 case(s) non enregistrée(s) : %2").arg(failed).arg(m_store.lastError()));
    QMessageBox::information(this, windowTitle(),
                             tr("%1 case(s) ajoutée(s), %2 ignorée(s) (repos ou déjà pointées).").arg(added).arg(skipped));
}

void MainWindow::modifyPresence(const QString &personId)
{
    const CloudPerson *person = findPerson(personId);
    if (!person)
        return;

    const CloudMarks marks = m_marks.value(personId);
    if (marks.isEmpty()) {
        QMessageBox::information(this, windowTitle(),
                                 tr("Aucune case pointée ce mois-ci pour %1.").arg(person->name));
        return;
    }

    PresenceDialog dialog(PresenceDialog::Mode::Modify, person->name, selectedYear(), selectedMonth(), marks, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QDate date = dialog.dates().first();
    reportWriteResult(m_store.setMark(personId, date, dialog.status()), tr("Modification"));
    refresh();
}

void MainWindow::deletePerson(const QString &personId)
{
    const CloudPerson *person = findPerson(personId);
    if (!person)
        return;

    const QString name = person->name;
    const QString kindLabel = kindDef(m_kind).singular;

    QMessageBox box(this);
    box.setWindowTitle(windowTitle());
    box.setIcon(QMessageBox::Question);
    box.setText(tr("Que voulez-vous faire pour %1 (%2) ?").arg(name, kindLabel));
    box.setInformativeText(tr("Effacer l'historique garde la fiche et supprime toutes ses cases pointées.\n"
                              "Supprimer la personne retire la fiche et tout son historique."));
    QPushButton *clearButton = box.addButton(tr("Effacer l'historique"), QMessageBox::ActionRole);
    QPushButton *removeButton = box.addButton(tr("Supprimer la personne"), QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    const QAbstractButton *clicked = box.clickedButton();
    if (clicked != clearButton && clicked != removeButton)
        return;

    const bool clearOnly = clicked == clearButton;
    const auto answer = QMessageBox::question(
        this, windowTitle(),
        clearOnly ? tr("Effacer tout l'historique de %1 ? Cette action est définitive.").arg(name)
                  : tr("Supprimer %1 et tout son historique ? Cette action est définitive.").arg(name));
    if (answer != QMessageBox::Yes)
        return;

    if (clearOnly)
        reportWriteResult(m_store.clearMarks(personId), tr("Effacement de l'historique"));
    else
        reportWriteResult(m_store.deletePerson(personId), tr("Suppression"));
    refresh();
}
