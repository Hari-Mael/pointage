#include "mainwindow.h"

#include <QColor>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

#include "accountpanel.h"
#include "classboarddialog.h"
#include "contractboarddialog.h"
#include "exporter.h"
#include "fields.h"
#include "glass.h"
#include "headerbar.h"
#include "persondialog.h"
#include "photo.h"
#include "presencedialog.h"
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
constexpr int kAbsenceRecidive = 2;

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

class DayChart : public QWidget {
public:
    explicit DayChart(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(180);
    }

    void setValues(const std::vector<int> &values)
    {
        m_values = values;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const int count = static_cast<int>(m_values.size());
        if (count == 0)
            return;

        const int maxValue = std::max(1, *std::max_element(m_values.begin(), m_values.end()));
        const double slot = static_cast<double>(width()) / count;
        const double baseline = height() - 22.0;
        const double usable = baseline - 8.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(52, 211, 153));
        for (int i = 0; i < count; ++i) {
            const double h = usable * m_values[static_cast<size_t>(i)] / maxValue;
            painter.drawRoundedRect(QRectF(i * slot + slot * 0.2, baseline - h, slot * 0.6, h), 3, 3);
        }

        painter.setPen(QColor("#9fbfb4"));
        for (int i = 0; i < count; i += 5) {
            painter.drawText(QRectF(i * slot, baseline + 4, slot * 5, 16), Qt::AlignLeft,
                             QString::number(i + 1));
        }
    }

private:
    std::vector<int> m_values;
};

class RateRing : public QWidget {
public:
    explicit RateRing(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(160, 160);
    }

    void setValue(int value)
    {
        m_value = std::clamp(value, 0, 100);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const double side = std::min(width(), height()) - 24.0;
        const QRectF rect((width() - side) / 2.0, (height() - side) / 2.0, side, side);

        QPen track(QColor(255, 255, 255, 40));
        track.setWidth(14);
        track.setCapStyle(Qt::RoundCap);
        painter.setPen(track);
        painter.drawArc(rect, 0, 360 * 16);

        QPen arc(QColor(52, 211, 153));
        arc.setWidth(14);
        arc.setCapStyle(Qt::RoundCap);
        painter.setPen(arc);
        painter.drawArc(rect, 90 * 16, -m_value * 360 * 16 / 100);

        QFont font = painter.font();
        font.setPointSize(20);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(QColor("#f4fbf8"));
        painter.drawText(rect, Qt::AlignCenter, QStringLiteral("%1%").arg(m_value));
    }

private:
    int m_value = 0;
};

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
    m_search = m_header->searchEdit();

    const QDate today = QDate::currentDate();
    m_monthBox->setCurrentIndex(today.month() - 1);
    m_yearBox->setValue(today.year());

    connect(m_monthBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refresh(); });
    connect(m_yearBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { refresh(); });
    connect(m_avatarButton, &QPushButton::clicked, this, &MainWindow::changeUserAvatar);
    connect(m_header, &HeaderBar::settingsRequested, this, &MainWindow::openSettings);
    connect(m_header, &HeaderBar::homeRequested, this, [this]() { refresh(); });
    connect(m_header, &HeaderBar::menuToggleRequested, this, &MainWindow::toggleSidebar);
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applySearch);
    return m_header;
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(buildTableCard(), 1);
    return page;
}

QWidget *MainWindow::buildActionsPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->setAlignment(Qt::AlignTop);
    layout->addWidget(makeTitle(tr("Actions"), page));

    auto *employeeBoard = new ContractBoardDialog(m_store, page);
    auto *classBoard = new ClassBoardDialog(m_store, page);
    employeeBoard->hide();
    classBoard->hide();
    connect(employeeBoard, &ContractBoardDialog::changed, this, &MainWindow::refresh);
    connect(classBoard, &ClassBoardDialog::changed, this, &MainWindow::refresh);

    QFrame *employeeCard = makeCard(page);
    auto *employeeLayout = new QVBoxLayout(employeeCard);
    employeeLayout->setContentsMargins(20, 20, 20, 20);
    employeeLayout->setSpacing(8);
    employeeLayout->addWidget(makeTitle(tr("Employés"), employeeCard));
    employeeLayout->addWidget(makeMuted(tr("Créer une fiche employé et définir son contrat."), employeeCard));
    employeeLayout->addStretch(1);
    auto *addEmployee = new QPushButton(tr("Ajouter employé"), employeeCard);
    addEmployee->setMinimumHeight(44);
    employeeLayout->addWidget(addEmployee);

    QFrame *studentCard = makeCard(page);
    auto *studentLayout = new QVBoxLayout(studentCard);
    studentLayout->setContentsMargins(20, 20, 20, 20);
    studentLayout->setSpacing(8);
    studentLayout->addWidget(makeTitle(tr("Élèves"), studentCard));
    studentLayout->addWidget(makeMuted(tr("Créer une fiche élève dans une classe, ou déplacer un élève entre classes."),
                                       studentCard));
    studentLayout->addStretch(1);
    auto *addStudent = new QPushButton(tr("Ajouter élève"), studentCard);
    addStudent->setMinimumHeight(44);
    studentLayout->addWidget(addStudent);

    connect(addEmployee, &QPushButton::clicked, this, [employeeBoard, classBoard]() {
        classBoard->hide();
        employeeBoard->setVisible(!employeeBoard->isVisible());
    });
    connect(addStudent, &QPushButton::clicked, this, [employeeBoard, classBoard]() {
        employeeBoard->hide();
        classBoard->setVisible(!classBoard->isVisible());
    });

    auto *cards = new QHBoxLayout;
    cards->setSpacing(12);
    cards->addWidget(employeeCard, 1);
    cards->addWidget(studentCard, 1);
    layout->addLayout(cards);

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

    const QStringList titles{tr("Présence"), tr("Absence"), tr("Retard"), tr("Notifications")};
    auto *kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(12);
    for (int i = 0; i < 4; ++i) {
        QFrame *card = makeCard(page);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(18, 14, 18, 14);
        cardLayout->setSpacing(4);
        cardLayout->addWidget(makeMuted(titles.at(i), card));
        m_kpiValue[static_cast<size_t>(i)] = makeTitle(QStringLiteral("0"), card);
        m_kpiSub[static_cast<size_t>(i)] = makeMuted(QString(), card);
        cardLayout->addWidget(m_kpiValue[static_cast<size_t>(i)]);
        cardLayout->addWidget(m_kpiSub[static_cast<size_t>(i)]);
        kpiRow->addWidget(card, 1);
    }
    layout->addLayout(kpiRow);

    auto *middle = new QHBoxLayout;
    middle->setSpacing(12);

    QFrame *chartCard = makeCard(page);
    auto *chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setContentsMargins(20, 16, 20, 16);
    chartLayout->addWidget(makeTitle(tr("Présences par jour"), chartCard));
    m_chart = new DayChart(chartCard);
    chartLayout->addWidget(m_chart, 1);
    middle->addWidget(chartCard, 2);

    QFrame *rateCard = makeCard(page);
    auto *rateLayout = new QVBoxLayout(rateCard);
    rateLayout->setContentsMargins(20, 16, 20, 16);
    rateLayout->addWidget(makeTitle(tr("Taux de présence du mois"), rateCard));
    m_rate = new RateRing(rateCard);
    rateLayout->addWidget(m_rate, 1, Qt::AlignCenter);
    middle->addWidget(rateCard, 1);
    layout->addLayout(middle, 1);

    QFrame *notifyCard = makeCard(page);
    auto *notifyLayout = new QVBoxLayout(notifyCard);
    notifyLayout->setContentsMargins(20, 16, 20, 16);
    notifyLayout->addWidget(makeTitle(tr("Notifications"), notifyCard));
    auto *notifyScroll = new QScrollArea(notifyCard);
    notifyScroll->setWidgetResizable(true);
    notifyScroll->setFrameShape(QFrame::NoFrame);
    notifyScroll->setMinimumHeight(160);
    notifyScroll->setMaximumHeight(240);
    notifyScroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));
    notifyScroll->viewport()->setAutoFillBackground(false);
    m_notifyBody = makeMuted(QString(), notifyScroll);
    m_notifyBody->setTextFormat(Qt::PlainText);
    m_notifyBody->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    notifyScroll->setWidget(m_notifyBody);
    notifyLayout->addWidget(notifyScroll);
    layout->addWidget(notifyCard);

    QFrame *tools = makeCard(page);
    auto *toolsLayout = new QHBoxLayout(tools);
    toolsLayout->setContentsMargins(20, 16, 20, 16);
    toolsLayout->setSpacing(10);
    toolsLayout->addWidget(makeMuted(tr("Import / export des listes."), tools), 1);
    auto *importButton = new QPushButton(tr("Importer Excel"), tools);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importRoster);
    auto *exportButton = new QPushButton(tr("Exporter Excel"), tools);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportRoster);
    toolsLayout->addWidget(importButton);
    toolsLayout->addWidget(exportButton);
    layout->addWidget(tools);
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

    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("settingsContent"));
    content->setStyleSheet(QStringLiteral("QWidget#settingsContent { background: transparent; }"));
    auto *row = new QHBoxLayout(content);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(12);

    QFrame *settingsCard = makeCard(content);
    auto *settingsLayout = new QVBoxLayout(settingsCard);
    settingsLayout->setContentsMargins(20, 20, 20, 20);
    settingsLayout->setSpacing(10);
    settingsLayout->addWidget(makeTitle(tr("Établissement, logo et créneaux"), settingsCard));
    settingsLayout->addWidget(new SettingsDialog(settingsCard));
    row->addWidget(settingsCard, 1);

    QFrame *accountCard = makeCard(content);
    auto *accountLayout = new QVBoxLayout(accountCard);
    accountLayout->setContentsMargins(20, 20, 20, 20);
    accountLayout->setSpacing(10);
    accountLayout->addWidget(makeTitle(tr("Identifiant et mot de passe"), accountCard));
    accountLayout->addWidget(new AccountPanel(m_store, m_userEmail, accountCard));
    row->addWidget(accountCard, 1);

    scroll->setWidget(content);
    layout->addWidget(scroll, 1);
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

void MainWindow::refresh()
{
    const int year = selectedYear();
    const int month = selectedMonth();
    const int days = QDate(year, month, 1).daysInMonth();
    const int actionsCol = kColFirstDay + days;

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
                cell->setText(QStringLiteral("P"));
                cell->setForeground(QColor("#6ee7b7"));
                cell->setBackground(QColor("#103d2b"));
            } else if (status == QChar('A')) {
                cell->setText(QStringLiteral("A"));
                cell->setForeground(QColor("#fda4af"));
                cell->setBackground(QColor("#4a1c2a"));
            } else if (status == QChar('R')) {
                cell->setText(QStringLiteral("R"));
                cell->setForeground(QColor("#fcd34d"));
                cell->setBackground(QColor("#453614"));
            }
            m_table->setItem(row, kColFirstDay + day - 1, cell);
        }

        m_table->setCellWidget(row, actionsCol, makeActionWidget(person.id));
    }

    applySearch();
    updateAnalysis(year, month);

    const int pending = static_cast<int>(m_store.pendingChanges());
    m_header->setPendingCount(pending);
    if (m_pendingLabel)
        m_pendingLabel->setText(tr("Modifications en attente d'envoi : %1").arg(pending));
}

void MainWindow::updateAnalysis(int year, int month)
{
    const QDate today = QDate::currentDate();
    const QDate first(year, month, 1);
    const int days = first.daysInMonth();
    const QDate last(year, month, days);
    int lastDay = 0;
    if (first <= today)
        lastDay = (last <= today) ? days : today.day();

    std::vector<int> presentByDay(static_cast<size_t>(days) + 1, 0);
    int presentTotal = 0;
    int absentTotal = 0;
    int lateTotal = 0;
    int possibleTotal = 0;
    QStringList notifications;

    for (const CloudPerson &person : m_people) {
        const CloudMarks marks = m_marks.value(person.id);
        int absences = 0;
        int lates = 0;
        int unmarked = 0;

        for (int day = 1; day <= lastDay; ++day) {
            if (isRestDay(QDate(year, month, day)))
                continue;
            ++possibleTotal;
            const QChar status = marks.value(day);
            if (status == QChar('P') || status == QChar('R')) {
                ++presentTotal;
                ++presentByDay[static_cast<size_t>(day)];
                if (status == QChar('R')) {
                    ++lates;
                    ++lateTotal;
                }
            } else if (status == QChar('A')) {
                ++absences;
                ++absentTotal;
            } else {
                ++unmarked;
            }
        }

        QStringList reasons;
        if (unmarked > 0)
            reasons << tr("%1 jour(s) non pointé(s)").arg(unmarked);
        if (lates > 0)
            reasons << tr("retard : %1").arg(lates);
        if (absences >= kAbsenceRecidive)
            reasons << tr("absence récidive : %1").arg(absences);
        if (!reasons.isEmpty())
            notifications << QStringLiteral("%1 — %2").arg(person.name, reasons.join(QStringLiteral(", ")));
    }

    const int rate = possibleTotal > 0 ? presentTotal * 100 / possibleTotal : 0;

    m_kpiValue[0]->setText(QString::number(presentTotal));
    m_kpiSub[0]->setText(tr("sur %1 jour(s) à pointer").arg(possibleTotal));
    m_kpiValue[1]->setText(QString::number(absentTotal));
    m_kpiSub[1]->setText(tr("cases marquées absent"));
    m_kpiValue[2]->setText(QString::number(lateTotal));
    m_kpiSub[2]->setText(tr("cases marquées retard"));
    m_kpiValue[3]->setText(QString::number(notifications.size()));
    m_kpiSub[3]->setText(tr("personne(s) à suivre"));

    static_cast<RateRing *>(m_rate)->setValue(rate);

    std::vector<int> series(static_cast<size_t>(days), 0);
    for (int day = 1; day <= days; ++day)
        series[static_cast<size_t>(day - 1)] = presentByDay[static_cast<size_t>(day)];
    static_cast<DayChart *>(m_chart)->setValues(series);

    m_notifyBody->setText(notifications.isEmpty()
                              ? tr("Aucune notification pour cette période.")
                              : notifications.join(QLatin1Char('\n')));
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
    m_sidebar->setCurrentSection(Sidebar::Settings);
    m_pages->setCurrentIndex(4);
}

void MainWindow::toggleSidebar()
{
    m_sidebar->setVisible(!m_sidebar->isVisible());
}

void MainWindow::liveUpdate()
{
    refresh();
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
