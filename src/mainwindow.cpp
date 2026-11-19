#include "mainwindow.h"

#include <QColor>
#include <QComboBox>
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
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "accountpanel.h"
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
