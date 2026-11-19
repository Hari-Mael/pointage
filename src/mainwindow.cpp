#include "mainwindow.h"

#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QComboBox>

#include "classboarddialog.h"
#include "contractboarddialog.h"
#include "dashboarddialog.h"
#include "exporter.h"
#include "fields.h"
#include "glass.h"
#include "headerbar.h"
#include "persondialog.h"
#include "photo.h"
#include "presencedialog.h"
#include "schedule.h"
#include "schoolinfo.h"
#include "settingsdialog.h"
#include "sidebar.h"
#include "studentreport.h"

namespace {

constexpr int kColCheck = 0;
constexpr int kColId = 1;
constexpr int kColName = 2;
constexpr int kColFirstDay = 3;
constexpr int kDayColumnWidth = 36;
constexpr int kActionsColumnWidth = 540;
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
    return new GlassCard(parent);
}

QLabel *makeTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("title"));
    return label;
}

QLabel *makeMuted(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("muted"));
    label->setWordWrap(true);
    return label;
}

QFrame *makeInfoCard(const QString &title, const QString &description, QPushButton *button, QWidget *parent)
{
    QFrame *card = makeCard(parent);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);
    layout->addWidget(makeTitle(title, card));
    layout->addWidget(makeMuted(description, card));
    layout->addStretch(1);
    button->setParent(card);
    layout->addWidget(button);
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
    m_store.fetchAvatar(m_userAvatar);
    buildUi();
    refresh();
}

void MainWindow::reportWriteResult(bool ok, const QString &action)
{
    if (!ok)
        QMessageBox::warning(this, windowTitle(), tr("%1 impossible : %2").arg(action, m_store.lastError()));
}

void MainWindow::saveReport(const QString &defaultName, const QString &content)
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Enregistrer en CSV (Excel)"), defaultName,
                                                      tr("Fichiers CSV (*.csv)"));
    if (path.isEmpty())
        return;
    QString error;
    if (Exporter::writeCsv(path, content, error))
        QMessageBox::information(this, windowTitle(), tr("Fichier enregistré : %1").arg(path));
    else
        QMessageBox::warning(this, windowTitle(), tr("Enregistrement impossible : %1").arg(error));
}

void MainWindow::buildUi()
{
    auto *shell = new GlassBackdrop(this);
    auto *shellLayout = new QHBoxLayout(shell);
    shellLayout->setContentsMargins(12, 12, 12, 12);
    shellLayout->setSpacing(12);

    m_sidebar = new Sidebar(shell);
    shellLayout->addWidget(m_sidebar);

    auto *main = new QWidget(shell);
    auto *mainLayout = new QVBoxLayout(main);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(buildHeader());

    m_pages = new QStackedWidget(main);
    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildActionsPage());
    m_pages->addWidget(buildAnalysisPage());
    m_pages->addWidget(buildPlansPage());
    m_pages->addWidget(buildSettingsPage());
    mainLayout->addWidget(m_pages, 1);
    shellLayout->addWidget(main, 1);
    setCentralWidget(shell);

    connect(m_sidebar, &Sidebar::sectionChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_sidebar, &Sidebar::logoutRequested, this, &MainWindow::logoutRequested);

    const QList<QWidget *> sections{m_sidebar, m_header, m_pages};
    GlassAnim::staggerIn(sections);

    m_liveTimer = new QTimer(this);
    m_liveTimer->setInterval(30000);
    connect(m_liveTimer, &QTimer::timeout, this, &MainWindow::liveUpdate);
    m_liveTimer->start();
}

QWidget *MainWindow::buildHeader()
{
    m_header = new HeaderBar(m_userName, m_userEmail, Photo::avatar(m_userAvatar, m_userName, 40));
    m_monthBox = m_header->monthBox();
    m_yearBox = m_header->yearBox();
    m_avatarButton = m_header->avatarButton();

    const QDate today = QDate::currentDate();
    m_monthBox->setCurrentIndex(today.month() - 1);
    m_yearBox->setValue(today.year());

    connect(m_monthBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refresh(); });
    connect(m_yearBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { refresh(); });
    connect(m_avatarButton, &QPushButton::clicked, this, &MainWindow::changeUserAvatar);
    connect(m_header, &HeaderBar::settingsRequested, this, &MainWindow::openSettings);
    connect(m_header, &HeaderBar::homeRequested, this, [this]() { refresh(); });
    return m_header;
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(buildSearchBar());
    layout->addWidget(buildTableCard(), 1);
    return page;
}

QWidget *MainWindow::buildActionsPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeTitle(tr("Actions"), page));

    auto *employeeBoard = new ContractBoardDialog(m_store, page);
    auto *classBoard = new ClassBoardDialog(m_store, page);
    employeeBoard->hide();
    classBoard->hide();
    connect(employeeBoard, &ContractBoardDialog::changed, this, &MainWindow::refresh);
    connect(classBoard, &ClassBoardDialog::changed, this, &MainWindow::refresh);

    auto *addEmployee = new QPushButton(tr("Ajouter employé"), page);
    auto *addStudent = new QPushButton(tr("Ajouter élève"), page);
    connect(addEmployee, &QPushButton::clicked, this, [employeeBoard, classBoard]() {
        classBoard->hide();
        employeeBoard->setVisible(!employeeBoard->isVisible());
    });
    connect(addStudent, &QPushButton::clicked, this, [employeeBoard, classBoard]() {
        employeeBoard->hide();
        classBoard->setVisible(!classBoard->isVisible());
    });

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addWidget(addEmployee);
    buttons->addWidget(addStudent);
    buttons->addStretch(1);
    layout->addLayout(buttons);

    layout->addWidget(employeeBoard, 1);
    layout->addWidget(classBoard, 1);
    return page;
}

QWidget *MainWindow::buildAnalysisPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeTitle(tr("Analyse"), page));
    layout->addWidget(buildStats());

    QFrame *tools = makeCard(page);
    auto *toolsLayout = new QHBoxLayout(tools);
    toolsLayout->setContentsMargins(20, 16, 20, 16);
    toolsLayout->setSpacing(10);
    toolsLayout->addWidget(makeMuted(tr("Import / export des listes et tableau de bord détaillé."), tools), 1);

    auto *importButton = new QPushButton(tr("Importer Excel"), tools);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importRoster);
    auto *exportButton = new QPushButton(tr("Exporter Excel"), tools);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportRoster);
    auto *dashboardButton = new QPushButton(tr("Tableau de bord détaillé"), tools);
    connect(dashboardButton, &QPushButton::clicked, this, &MainWindow::openDashboard);

    toolsLayout->addWidget(importButton);
    toolsLayout->addWidget(exportButton);
    toolsLayout->addWidget(dashboardButton);
    layout->addWidget(tools);
    layout->addStretch(1);
    return page;
}

QWidget *MainWindow::buildPlansPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeTitle(tr("Plans"), page));

    QFrame *restCard = makeCard(page);
    auto *restLayout = new QVBoxLayout(restCard);
    restLayout->setContentsMargins(20, 16, 20, 16);
    restLayout->addWidget(makeTitle(tr("Planning hebdomadaire"), restCard));
    restLayout->addWidget(makeMuted(tr("Les jours de repos ne peuvent pas recevoir de pointage."), restCard));
    restLayout->addWidget(buildRestBar());
    layout->addWidget(restCard);

    QFrame *syncCard = makeCard(page);
    auto *syncLayout = new QVBoxLayout(syncCard);
    syncLayout->setContentsMargins(20, 16, 20, 16);
    syncLayout->addWidget(makeTitle(tr("Synchronisation"), syncCard));
    m_pendingLabel = makeMuted(QString(), syncCard);
    syncLayout->addWidget(m_pendingLabel);
    layout->addWidget(syncCard);

    layout->addStretch(1);
    return page;
}

QWidget *MainWindow::buildSettingsPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeTitle(tr("Paramètres"), page));

    auto *schoolButton = new QPushButton(tr("Ouvrir les paramètres"), page);
    connect(schoolButton, &QPushButton::clicked, this, &MainWindow::openSettings);

    auto *accountButton = new QPushButton(tr("Bientôt disponible"), page);
    accountButton->setEnabled(false);

    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    row->addWidget(makeInfoCard(tr("Établissement, logo et créneaux"),
                                tr("Nom de l'école, logo, plages horaires et confidentialité."),
                                schoolButton, page), 1);
    row->addWidget(makeInfoCard(tr("Identifiant et mot de passe"),
                                tr("La modification du compte nécessite une route côté serveur, "
                                   "qui n'est pas encore branchée."),
                                accountButton, page), 1);
    layout->addLayout(row);
    layout->addStretch(1);
    return page;
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
    const int actionsCol = kColFirstDay + days;
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
    headers << tr("Actions");
    m_table->setHorizontalHeaderLabels(headers);

    m_table->horizontalHeader()->setSectionResizeMode(kColCheck, QHeaderView::Fixed);
    m_table->setColumnWidth(kColCheck, 36);
    m_table->horizontalHeader()->setSectionResizeMode(kColId, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(actionsCol, QHeaderView::Fixed);
    m_table->setColumnWidth(actionsCol, kActionsColumnWidth);
    for (int col = kColFirstDay; col < actionsCol; ++col)
        m_table->setColumnWidth(col, kDayColumnWidth);

    const int rowCount = static_cast<int>(m_people.size());
    m_table->setRowCount(rowCount);

    int totalPresent = 0;
    int totalAbsent = 0;
    for (int row = 0; row < rowCount; ++row) {
        const CloudPerson &person = m_people[static_cast<size_t>(row)];
        const CloudMarks marks = m_marks.value(person.id);

        auto *check = new QTableWidgetItem;
        check->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        check->setCheckState(Qt::Unchecked);
        m_table->setItem(row, kColCheck, check);

        QTableWidgetItem *idItem = makeItem(person.code, Qt::AlignLeft);
        idItem->setData(Qt::UserRole, person.id);
        m_table->setItem(row, kColId, idItem);
        const QString displayName = person.className.isEmpty()
                                        ? person.name
                                        : QStringLiteral("%1  ·  %2").arg(person.name, person.className);
        QTableWidgetItem *nameItem = makeItem(displayName, Qt::AlignLeft);
        nameItem->setIcon(QIcon(Photo::avatar(person.photo, person.name, 30)));
        m_table->setItem(row, kColName, nameItem);

        for (int day = 1; day <= days; ++day) {
            QTableWidgetItem *cell = makeItem(QString());
            const QDate date(year, month, day);
            const QChar status = marks.value(day);
            if (isRestDay(date)) {
                cell->setBackground(QColor("#1a2622"));
            } else if (status == QChar('P')) {
                ++totalPresent;
                cell->setText(QStringLiteral("P"));
                cell->setForeground(QColor("#6ee7b7"));
                cell->setBackground(QColor("#103d2b"));
            } else if (status == QChar('A')) {
                ++totalAbsent;
                cell->setText(QStringLiteral("A"));
                cell->setForeground(QColor("#fda4af"));
                cell->setBackground(QColor("#4a1c2a"));
            } else if (status == QChar('R')) {
                ++totalPresent;
                cell->setText(QStringLiteral("R"));
                cell->setForeground(QColor("#fcd34d"));
                cell->setBackground(QColor("#453614"));
            }
            m_table->setItem(row, kColFirstDay + day - 1, cell);
        }

        m_table->setCellWidget(row, actionsCol, makeActionWidget(person.id));
    }

    GlassAnim::countUp(m_statPresent, totalPresent, [this](int v) { return tr("Présences : %1").arg(v); });
    GlassAnim::countUp(m_statAbsent, totalAbsent, [this](int v) { return tr("Absences : %1").arg(v); });
    GlassAnim::countUp(m_statEmp, static_cast<int>(employees), [this](int v) { return tr("Employés : %1").arg(v); });
    GlassAnim::countUp(m_statStu, static_cast<int>(students), [this](int v) { return tr("Élèves : %1").arg(v); });
    applySearch();

    const int pending = static_cast<int>(m_store.pendingChanges());
    m_header->setPendingCount(pending);
    if (m_pendingLabel)
        m_pendingLabel->setText(tr("Modifications en attente d'envoi : %1").arg(pending));
    if (m_dashboard && m_dashboard->isVisible())
        m_dashboard->setSnapshot(buildSnapshot());
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
    else if (current == QLatin1String("A"))
        next = QChar('R');

    reportWriteResult(m_store.setMark(personId, date, next), tr("Enregistrement"));
    refresh();
}

QWidget *MainWindow::makeActionWidget(const QString &personId)
{
    auto *box = new QWidget;
    auto *layout = new QHBoxLayout(box);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(6);

    auto *editPersonButton = new QPushButton(tr("Modifier la fiche"), box);
    editPersonButton->setProperty("small", QStringLiteral("true"));
    auto *insertButton = new QPushButton(tr("Insérer"), box);
    insertButton->setProperty("small", QStringLiteral("true"));
    auto *modifyButton = new QPushButton(tr("Modifier"), box);
    modifyButton->setProperty("small", QStringLiteral("true"));
    auto *deleteButton = new QPushButton(tr("Supprimer"), box);
    deleteButton->setProperty("variant", QStringLiteral("red"));
    deleteButton->setProperty("small", QStringLiteral("true"));
    auto *reportButton = new QPushButton(tr("Actions ▾"), box);
    reportButton->setProperty("small", QStringLiteral("true"));

    auto *menu = new QMenu(reportButton);
    menu->addAction(tr("Présence mensuelle (détail)"), [this, personId]() { exportMonthly(personId); });
    menu->addAction(tr("Taux de présentéisme"),
                    [this, personId]() { exportRate(personId, Reports::Rate::Presence); });
    menu->addAction(tr("Taux d'absence"), [this, personId]() { exportRate(personId, Reports::Rate::Absence); });
    menu->addAction(tr("Taux de retard"), [this, personId]() { exportRate(personId, Reports::Rate::Late); });
    menu->addSeparator();
    menu->addAction(tr("Rapport élève imprimable (PDF)"), [this, personId]() { openStudentReport(personId); });
    reportButton->setMenu(menu);

    layout->addWidget(editPersonButton);
    layout->addWidget(insertButton);
    layout->addWidget(modifyButton);
    layout->addWidget(deleteButton);
    layout->addWidget(reportButton);

    connect(editPersonButton, &QPushButton::clicked, this, [this, personId]() { editPerson(personId); });
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
    person.photo = dialog.photo();
    reportWriteResult(m_store.addPerson(person), tr("Ajout de la fiche"));
    refresh();
}

void MainWindow::editPerson(const QString &personId)
{
    const CloudPerson *found = findPerson(personId);
    if (!found)
        return;
    const CloudPerson original = *found;

    QStringList codes;
    for (const CloudPerson &person : m_all) {
        if (person.kind == original.kind && person.className == original.className && person.id != original.id)
            codes << person.code;
    }

    PersonDialog dialog(original.kind, original.code, codes, this);
    dialog.setEditMode();
    dialog.setValues(original.fields);
    dialog.setPhoto(original.photo);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const PersonRecord edited = dialog.person();
    CloudPerson updated = original;
    updated.code = edited.code;
    updated.name = edited.name;
    updated.fields = edited.fields;
    updated.photo = dialog.photo();
    reportWriteResult(m_store.updatePerson(updated), tr("Modification de la fiche"));
    refresh();
}

void MainWindow::openClassBoard()
{
    ClassBoardDialog dialog(m_store, this);
    connect(&dialog, &ClassBoardDialog::changed, this, &MainWindow::refresh);
    dialog.exec();
    refresh();
}

void MainWindow::openContractBoard()
{
    ContractBoardDialog dialog(m_store, this);
    connect(&dialog, &ContractBoardDialog::changed, this, &MainWindow::refresh);
    dialog.exec();
    refresh();
}

void MainWindow::openStudentReport(const QString &personId)
{
    const CloudPerson *person = findPerson(personId);
    if (!person)
        return;
    const QString schoolName = SchoolInfo::name();
    StudentReportDialog dialog(*person, m_marks.value(personId), selectedYear(), selectedMonth(), m_restMask,
                               QDate::currentDate(), schoolName, this);
    dialog.exec();
}

void MainWindow::importRoster()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Importer une liste (Excel / CSV)"), QString(),
                                                      tr("Fichiers CSV (*.csv);;Tous les fichiers (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, windowTitle(), tr("Lecture du fichier impossible : %1").arg(file.errorString()));
        return;
    }
    const QString content = Reports::decode(file.readAll());

    QStringList existing;
    for (const CloudPerson &person : m_all) {
        if (person.kind == m_kind)
            existing << person.code;
    }
    const Reports::RosterImport result = Reports::rosterFromCsv(content, m_kind, existing);

    int added = 0;
    int failed = 0;
    for (const CloudPerson &person : result.people) {
        if (m_store.addPerson(person))
            ++added;
        else
            ++failed;
    }
    refresh();

    const int errors = static_cast<int>(result.errors.size()) + failed;
    QString summary = tr("%1 fiche(s) importée(s), %2 déjà existante(s) ignorée(s), %3 erreur(s).")
                          .arg(added).arg(result.skippedExisting).arg(errors);
    if (!result.errors.isEmpty())
        summary += QStringLiteral("\n\n") + result.errors.mid(0, 10).join(QLatin1Char('\n'));
    QMessageBox::information(this, windowTitle(), summary);
}

void MainWindow::exportRoster()
{
    std::vector<CloudPerson> people;
    for (const CloudPerson &person : m_all) {
        if (person.kind == m_kind)
            people.push_back(person);
    }
    saveReport(QStringLiteral("%1.csv").arg(kindDef(m_kind).plural.toLower()), Reports::rosterCsv(people, m_kind));
}

void MainWindow::exportMonthly(const QString &personId)
{
    const CloudPerson *person = findPerson(personId);
    if (!person)
        return;
    const QString name = QStringLiteral("presence_%1_%2_%3.csv")
                             .arg(person->code, QString::number(selectedYear()))
                             .arg(selectedMonth(), 2, 10, QLatin1Char('0'));
    saveReport(name, Reports::monthlyDetailCsv(*person, m_marks.value(personId), selectedYear(), selectedMonth(),
                                              m_restMask, QDate::currentDate()));
}

void MainWindow::exportRate(const QString &personId, Reports::Rate rate)
{
    const CloudPerson *person = findPerson(personId);
    if (!person)
        return;
    const QString prefix = rate == Reports::Rate::Presence ? QStringLiteral("presenteisme")
                           : rate == Reports::Rate::Absence ? QStringLiteral("absence")
                                                            : QStringLiteral("retard");
    const QString name = QStringLiteral("taux_%1_%2_%3_%4.csv")
                             .arg(prefix, person->code, QString::number(selectedYear()))
                             .arg(selectedMonth(), 2, 10, QLatin1Char('0'));
    saveReport(name, Reports::rateCsv(*person, m_marks.value(personId), selectedYear(), selectedMonth(),
                                      m_restMask, QDate::currentDate(), rate));
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
        QMessageBox::warning(this, windowTitle(),
                             tr("%1 case(s) non enregistrée(s) : %2").arg(failed).arg(m_store.lastError()));
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

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

Dashboard::Snapshot MainWindow::buildSnapshot() const
{
    const QLocale french(QLocale::French);
    const QString title = tr("%1 — %2 %3")
                              .arg(kindDef(m_kind).plural, french.monthName(selectedMonth()),
                                   QString::number(selectedYear()));
    return Dashboard::compute(m_all, m_kind, m_marks, selectedYear(), selectedMonth(), m_restMask,
                              QDate::currentDate(), title);
}

void MainWindow::openDashboard()
{
    if (!m_dashboard)
        m_dashboard = new DashboardDialog(this);
    m_dashboard->setSnapshot(buildSnapshot());
    m_dashboard->show();
    m_dashboard->raise();
    m_dashboard->activateWindow();
}

void MainWindow::liveUpdate()
{
    if (!m_dashboard || !m_dashboard->isVisible())
        return;

    std::vector<CloudPerson> all;
    QHash<QString, CloudMarks> marks;
    int restMask = m_restMask;
    if (!m_store.fetchPeople(all) || !m_store.fetchMarks(selectedYear(), selectedMonth(), marks)
        || !m_store.fetchRestMask(restMask)) {
        return;
    }
    m_all = all;
    m_marks = marks;
    m_restMask = restMask;
    m_dashboard->setSnapshot(buildSnapshot());
}

void MainWindow::refreshAvatar()
{
    m_avatarButton->setIcon(QIcon(Photo::avatar(m_userAvatar, m_userName, 44)));
}

void MainWindow::changeUserAvatar()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Photo de profil"), QString(),
                                                      tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty())
        return;
    QString error;
    const QString encoded = Photo::encodeFile(path, error);
    if (encoded.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Photo non utilisable : %1").arg(error));
        return;
    }
    if (!m_store.setAvatar(encoded)) {
        reportWriteResult(false, tr("Enregistrement de la photo"));
        return;
    }
    m_userAvatar = encoded;
    refreshAvatar();
}
