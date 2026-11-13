#include "contractboarddialog.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "fields.h"
#include "persondialog.h"
#include "pipelinecards.h"

namespace {

const char *const kPersonMime = "application/x-pointage-person";
const char *const kContractLabel = "Type de contrat";
const char *const kPosteLabel = "Poste";

const QString kCdi = QStringLiteral("CDI");
const QString kCdd = QStringLiteral("CDD");
const QString kOther = QStringLiteral("Stage / Autre");
const QStringList kColumns{kCdi, kCdd, kOther};

int indexOfField(const char *label)
{
    const auto &fields = kindDef(kKindEmployee).fields;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].label == QLatin1String(label))
            return static_cast<int>(i);
    }
    return -1;
}

QString fieldOf(const CloudPerson &person, const char *label)
{
    const int index = indexOfField(label);
    return (index >= 0 && index < person.fields.size()) ? person.fields.at(index) : QString();
}

void setField(CloudPerson &person, const char *label, const QString &value)
{
    const int index = indexOfField(label);
    if (index < 0)
        return;
    while (person.fields.size() <= index)
        person.fields << QString();
    person.fields[index] = value;
}

QString columnFor(const QString &contract)
{
    if (contract == kCdi)
        return kCdi;
    if (contract == kCdd)
        return kCdd;
    return kOther;
}

QString contractFor(const QString &column)
{
    if (column == kCdi)
        return kCdi;
    if (column == kCdd)
        return kCdd;
    return QStringLiteral("Stage");
}

CardTone toneFor(const QString &column)
{
    if (column == kCdi)
        return {QColor("#1f6b4f"), QColor("#ffffff"), QColor("#1f6b4f")};
    if (column == kCdd)
        return {QColor("#f5c542"), QColor("#1d1d1f"), QColor("#f5c542")};
    return {QColor("#e8553d"), QColor("#ffffff"), QColor("#e8553d")};
}

QString nextEmployeeCode(const std::vector<CloudPerson> &people)
{
    int highest = 0;
    const QChar prefix = kindDef(kKindEmployee).prefix;
    for (const CloudPerson &person : people) {
        if (person.kind != kKindEmployee || !person.code.startsWith(prefix))
            continue;
        bool ok = false;
        const int number = person.code.mid(1).toInt(&ok);
        if (ok && number > highest)
            highest = number;
    }
    return QStringLiteral("%1%2").arg(prefix).arg(highest + 1, 3, 10, QLatin1Char('0'));
}

}

ContractList::ContractList(const QString &column, QWidget *parent)
    : QListWidget(parent), m_column(column)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::CopyAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFrameShape(QFrame::NoFrame);
    setSpacing(8);
    setStyleSheet(QStringLiteral("QListWidget { background: transparent; border: none; }"
                                 "QListWidget::item { background: transparent; border: none; }"));
}

bool ContractList::accepts(const QMimeData *data, const QObject *source) const
{
    return data && data->hasFormat(QLatin1String(kPersonMime)) && source != this;
}

QStringList ContractList::mimeTypes() const
{
    return {QLatin1String(kPersonMime)};
}

QMimeData *ContractList::mimeData(const QList<QListWidgetItem *> &items) const
{
    auto *data = new QMimeData;
    if (!items.isEmpty())
        data->setData(QLatin1String(kPersonMime), items.first()->data(Qt::UserRole).toString().toUtf8());
    return data;
}

void ContractList::dragEnterEvent(QDragEnterEvent *event)
{
    if (accepts(event->mimeData(), event->source()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ContractList::dragMoveEvent(QDragMoveEvent *event)
{
    if (accepts(event->mimeData(), event->source()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ContractList::dropEvent(QDropEvent *event)
{
    if (!accepts(event->mimeData(), event->source())) {
        event->ignore();
        return;
    }
    const QString personId = QString::fromUtf8(event->mimeData()->data(QLatin1String(kPersonMime)));
    event->setDropAction(Qt::CopyAction);
    event->accept();
    emit personDropped(personId, m_column);
}

ContractBoardDialog::ContractBoardDialog(CloudStore &store, QWidget *parent)
    : QDialog(parent), m_store(store)
{
    setWindowTitle(tr("Employés par type de contrat"));
    resize(1000, 640);
    setStyleSheet(QStringLiteral("QDialog { background: #f6f1e6; }"));

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *hint = new QLabel(tr("Glissez un employé d'un type de contrat vers un autre pour le changer. "
                               "Le matricule reste le même."),
                            this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));
    auto *content = new QWidget(scroll);
    content->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *row = new QHBoxLayout(content);
    row->setSpacing(14);
    for (const QString &column : kColumns)
        row->addWidget(makeColumn(column));
    row->addStretch(1);
    scroll->setWidget(content);
    layout->addWidget(scroll, 1);

    auto *undoRow = new QHBoxLayout;
    m_undoLabel = new QLabel(this);
    m_undoLabel->hide();
    m_undoButton = new QPushButton(tr("Annuler"), this);
    m_undoButton->setProperty("variant", QStringLiteral("ghost"));
    m_undoButton->hide();
    undoRow->addWidget(m_undoLabel, 1);
    undoRow->addWidget(m_undoButton);
    layout->addLayout(undoRow);

    auto *close = new QPushButton(tr("Fermer"), this);
    close->setProperty("variant", QStringLiteral("teal"));
    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    connect(m_undoButton, &QPushButton::clicked, this, &ContractBoardDialog::undoLastMove);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    reload();
}

QWidget *ContractBoardDialog::makeColumn(const QString &column)
{
    const CardTone tone = toneFor(column);
    ColumnParts parts = PipelineCards::makeColumn(column, tone.dot, this);
    m_counts.insert(column, parts.count);

    auto *list = new ContractList(column, parts.frame);
    list->setMinimumHeight(360);
    m_lists.insert(column, list);
    connect(list, &ContractList::personDropped, this, &ContractBoardDialog::onPersonDropped);
    parts.layout->addWidget(list, 1);

    auto *add = new QPushButton(tr("+ Ajouter un employé"), parts.frame);
    add->setProperty("variant", QStringLiteral("teal"));
    parts.layout->addWidget(add);
    connect(add, &QPushButton::clicked, this, [this, column]() { onAddEmployee(column); });
    return parts.frame;
}

void ContractBoardDialog::reload()
{
    m_employees.clear();
    std::vector<CloudPerson> all;
    if (!m_store.fetchPeople(all)) {
        QMessageBox::warning(this, windowTitle(), tr("Lecture des données impossible : %1").arg(m_store.lastError()));
        return;
    }
    for (const CloudPerson &person : all) {
        if (person.kind == kKindEmployee)
            m_employees.push_back(person);
    }

    for (auto it = m_lists.begin(); it != m_lists.end(); ++it) {
        ContractList *list = it.value();
        list->clear();
        const CardTone tone = toneFor(it.key());
        int count = 0;
        for (const CloudPerson &person : m_employees) {
            if (columnFor(fieldOf(person, kContractLabel)) != it.key())
                continue;
            const QString poste = fieldOf(person, kPosteLabel);
            auto *item = new QListWidgetItem(list);
            item->setData(Qt::UserRole, person.id);
            item->setSizeHint(QSize(0, 66));
            list->setItemWidget(item, PipelineCards::personCard(person, poste.isEmpty() ? person.code : poste,
                                                                tone, list));
            ++count;
        }
        m_counts.value(it.key())->setText(QString::number(count));
    }
}

const CloudPerson *ContractBoardDialog::findEmployee(const QString &personId) const
{
    for (const CloudPerson &person : m_employees) {
        if (person.id == personId)
            return &person;
    }
    return nullptr;
}

void ContractBoardDialog::onPersonDropped(const QString &personId, const QString &targetColumn)
{
    const CloudPerson *found = findEmployee(personId);
    if (!found)
        return;
    const QString currentColumn = columnFor(fieldOf(*found, kContractLabel));
    if (currentColumn == targetColumn)
        return;

    const CloudPerson moved = *found;
    const QString previous = fieldOf(moved, kContractLabel);
    CloudPerson updated = moved;
    setField(updated, kContractLabel, contractFor(targetColumn));
    if (!m_store.updatePerson(updated)) {
        QMessageBox::warning(this, windowTitle(), tr("Changement impossible : %1").arg(m_store.lastError()));
        return;
    }

    m_undoPersonId = moved.id;
    m_undoPreviousContract = previous;
    m_undoLabel->setText(tr("%1 passe en %2.").arg(moved.name, targetColumn));
    m_undoLabel->show();
    m_undoButton->show();
    reload();
    emit changed();
}

void ContractBoardDialog::undoLastMove()
{
    const CloudPerson *found = findEmployee(m_undoPersonId);
    if (!found)
        return;
    CloudPerson restored = *found;
    setField(restored, kContractLabel, m_undoPreviousContract);
    if (!m_store.updatePerson(restored)) {
        QMessageBox::warning(this, windowTitle(), tr("Annulation impossible : %1").arg(m_store.lastError()));
        return;
    }
    m_undoLabel->hide();
    m_undoButton->hide();
    reload();
    emit changed();
}

void ContractBoardDialog::onAddEmployee(const QString &column)
{
    QStringList codes;
    for (const CloudPerson &person : m_employees)
        codes << person.code;

    PersonDialog dialog(kKindEmployee, nextEmployeeCode(m_employees), codes, this);
    QStringList preset;
    for (size_t i = 0; i < kindDef(kKindEmployee).fields.size(); ++i)
        preset << QString();
    const int index = indexOfField(kContractLabel);
    if (index >= 0)
        preset[index] = contractFor(column);
    dialog.setValues(preset);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const PersonRecord created = dialog.person();
    CloudPerson person;
    person.kind = kKindEmployee;
    person.code = created.code;
    person.name = created.name;
    person.fields = created.fields;
    person.photo = dialog.photo();
    if (!m_store.addPerson(person)) {
        QMessageBox::warning(this, windowTitle(), tr("Ajout impossible : %1").arg(m_store.lastError()));
        return;
    }
    reload();
    emit changed();
}
