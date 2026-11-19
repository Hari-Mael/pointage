#include "mainwindow.h"

#include <QColor>
#include <QIcon>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

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
    auto *central = new GlassBackdrop(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    root->addWidget(buildHeader());
    root->addWidget(buildRestBar());
    root->addWidget(buildSearchBar());
    root->addWidget(buildTableCard(), 1);
    root->addWidget(buildFooter());

    setCentralWidget(central);

    QList<QWidget *> sections;
    for (QWidget *widget : central->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
        sections << widget;
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
    connect(m_header, &HeaderBar::dashboardRequested, this, &MainWindow::openDashboard);
    connect(m_header, &HeaderBar::settingsRequested, this, &MainWindow::openSettings);
    connect(m_header, &HeaderBar::homeRequested, this, [this]() { refresh(); });
    return m_header;
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

    connect(addEmployee, &QPushButton::clicked, this, &MainWindow::openContractBoard);
    connect(addStudent, &QPushButton::clicked, this, &MainWindow::openClassBoard);
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

    auto *importButton = new QPushButton(tr("Importer Excel"), card);
    importButton->setProperty("variant", QStringLiteral("ghost"));
    auto *exportButton = new QPushButton(tr("Exporter Excel"), card);
    exportButton->setProperty("variant", QStringLiteral("ghost"));
    tabs->addWidget(importButton);
    tabs->addWidget(exportButton);
    layout->addLayout(tabs);
    connect(m_tabEmp, &QPushButton::clicked, this, [this]() { setKind(kKindEmployee); });
    connect(m_tabStu, &QPushButton::clicked, this, [this]() { setKind(kKindStudent); });
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importRoster);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportRoster);

    m_table = new QTableWidget(card);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    connect(m_table, &QTableWidget::cellClicked, this, &MainWindow::onCellClicked);
    layout->addWidget(m_table, 1);

    layout->addWidget(buildStats());

    auto *hint = new QLabel(tr("Cliquez sur une case : vide → P → A → R (retard) → vide"), card);
    hint->setObjectName(QStringLiteral("muted"));
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);

    m_scheduleLabel = new QLabel(card);
    m_scheduleLabel->setObjectName(QStringLiteral("muted"));
    m_scheduleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_scheduleLabel);
    updateScheduleLabel();
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

void MainWindow::updateScheduleLabel()
{
    m_scheduleLabel->setText(tr("Créneaux : %1").arg(Schedule::load().summary()));
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

    GlassAnim::countUp(m_statPresent,
