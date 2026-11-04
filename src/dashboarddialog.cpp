#include "dashboarddialog.h"

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTime>
#include <QVBoxLayout>

#include "excelreport.h"

namespace {

QChart *makePieChart(const Dashboard::Snapshot &snapshot)
{
    auto *series = new QPieSeries;
    auto addSlice = [series](const QString &label, int value, const QColor &color) {
        if (value <= 0)
            return;
        QPieSlice *slice = series->append(QStringLiteral("%1 : %2").arg(label).arg(value), value);
        slice->setBrush(color);
        slice->setLabelVisible(false);
    };
    addSlice(QObject::tr("Présents"), snapshot.present, QColor("#14a39a"));
    addSlice(QObject::tr("Retards"), snapshot.late, QColor("#f0b040"));
    addSlice(QObject::tr("Absents"), snapshot.absent, QColor("#d63d43"));
    addSlice(QObject::tr("Non pointés"), snapshot.unmarked, QColor("#9aa5b1"));

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QObject::tr("Répartition des jours"));
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundVisible(false);
    return chart;
}

QChart *makeBarChart(const Dashboard::Snapshot &snapshot)
{
    auto *presence = new QBarSet(QObject::tr("Présentéisme"));
    auto *absence = new QBarSet(QObject::tr("Absentéisme"));
    auto *late = new QBarSet(QObject::tr("Retard"));
    presence->setBrush(QColor("#14a39a"));
    absence->setBrush(QColor("#d63d43"));
    late->setBrush(QColor("#f0b040"));

    QStringList categories;
    for (const Dashboard::PersonRow &person : snapshot.people) {
        categories << person.code;
        *presence << person.presenceRate * 100.0;
        *absence << person.absenceRate * 100.0;
        *late << person.lateRate * 100.0;
    }

    auto *series = new QBarSeries;
    series->append(presence);
    series->append(absence);
    series->append(late);

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QObject::tr("Taux par personne (%)"));
    chart->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setRange(0, 100);
    axisY->setLabelFormat(QStringLiteral("%.0f"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    return chart;
}

QString percent(double value)
{
    return QString::number(value * 100.0, 'f', 1).replace(QLatin1Char('.'), QLatin1Char(',')) + QStringLiteral(" %");
}

}

DashboardDialog::DashboardDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Tableau de bord"));
    resize(980, 680);
    setObjectName(QStringLiteral("loginDialog"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *header = new QHBoxLayout;
    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("title"));
    m_updated = new QLabel(this);
    m_updated->setObjectName(QStringLiteral("muted"));
    header->addWidget(m_title, 1);
    header->addWidget(m_updated);
    layout->addLayout(header);

    auto *kpis = new QHBoxLayout;
    kpis->addWidget(makeKpi(tr("Taux de présentéisme"), m_presence, this));
    kpis->addWidget(makeKpi(tr("Taux d'absentéisme"), m_absence, this));
    kpis->addWidget(makeKpi(tr("Taux de retard"), m_late, this));
    layout->addLayout(kpis);

    m_chartsHost = new QWidget(this);
    m_chartsLayout = new QVBoxLayout(m_chartsHost);
    m_chartsLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chartsHost, 1);

    auto *buttons = new QHBoxLayout;
    auto *exportButton = new QPushButton(tr("Exporter le rapport Excel (avec graphiques)"), this);
    exportButton->setProperty("variant", QStringLiteral("teal"));
    auto *closeButton = new QPushButton(tr("Fermer"), this);
    closeButton->setProperty("variant", QStringLiteral("ghost"));
    buttons->addStretch(1);
    buttons->addWidget(exportButton);
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);

    connect(exportButton, &QPushButton::clicked, this, &DashboardDialog::exportExcel);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

QWidget *DashboardDialog::makeKpi(const QString &caption, QLabel *&valueLabel, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 14);
    auto *captionLabel = new QLabel(caption, card);
    captionLabel->setObjectName(QStringLiteral("subtitle"));
    valueLabel = new QLabel(QStringLiteral("0 %"), card);
    valueLabel->setStyleSheet(QStringLiteral("font-size: 26pt; font-weight: 800; color: #0b3b5c;"));
    layout->addWidget(captionLabel);
    layout->addWidget(valueLabel);
    return card;
}

void DashboardDialog::setSnapshot(const Dashboard::Snapshot &snapshot)
{
    m_snapshot = snapshot;
    m_title->setText(snapshot.title);
    m_updated->setText(tr("Mise à jour : %1").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss"))));
    m_presence->setText(percent(snapshot.presenceRate));
    m_absence->setText(percent(snapshot.absenceRate));
    m_late->setText(percent(snapshot.lateRate));
    rebuildCharts();
}

void DashboardDialog::rebuildCharts()
{
    while (QLayoutItem *item = m_chartsLayout->takeAt(0)) {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    auto *row = new QWidget(m_chartsHost);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(new QChartView(makePieChart(m_snapshot)), 1);
    rowLayout->addWidget(new QChartView(makeBarChart(m_snapshot)), 2);
    m_chartsLayout->addWidget(row);
    row->show();
}

void DashboardDialog::exportExcel()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Exporter le rapport Excel"),
                                                      QStringLiteral("rapport_pointage.xlsx"),
                                                      tr("Classeur Excel (*.xlsx)"));
    if (path.isEmpty())
        return;
    QString error;
    if (ExcelReport::writeMonthlyReport(path, m_snapshot, error))
        QMessageBox::information(this, windowTitle(), tr("Rapport enregistré : %1").arg(path));
    else
        QMessageBox::warning(this, windowTitle(), tr("Export impossible : %1").arg(error));
}
