#include "contractboarddialog.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "fields.h"
#include "persondialog.h"

namespace {

const char *const kPersonMime = "application/x-pointage-person";
const char *const kContractLabel = "Type de contrat";

const QStringList kColumns{QStringLiteral("CDI"), QStringLiteral("CDD"), QStringLiteral("Intérim")};

int contractIndex()
{
    const auto &fields = kindDef(kKindEmployee).fields;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].label == QLatin1String(kContractLabel))
            return static_cast<int>(i);
    }
    return -1;
}

QString contractOf(const CloudPerson &person)
{
    const int index = contractIndex();
    return (index >= 0 && index < person.fields.size()) ? person.fields.at(index) : QString();
}

void setContract(CloudPerson &person, const QString &contract)
{
    const int index = contractIndex();
    if (index < 0)
        return;
    while (person.fields.size() <= index)
        person.fields << QString();
    person.fields[index] = contract;
}

QString columnFor(const QString &contract)
{
    return kColumns.contains(contract) ? contract : QString();
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

ContractList::ContractList(const QString &contract, QWidget *parent)
    : QListWidget(parent), m_contract(contract)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::CopyAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setMinimumWidth(190);
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
    emit personDropped(personId, m_contract);
}

ContractBoardDialog::ContractBoardDialog(CloudStore &store, QWidget *parent)
    : QDialog(parent), m_store(store)
{
    setWindowTitle(tr("Employés par type de contrat"));
    resize(1000, 620);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *hint = new QLabel(tr("Glissez un employé d'un type de contrat vers un autre pour le changer. "
                               "Le matricule reste le même."),
                            this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget(scroll);
    auto *row = new QHBoxLayout(content);
    row->setSpacing(12);
    for (const QString &contract : kColumns)
        row->addWidget(makeColumn(contract, contract));
    row->addWidget(makeColumn(QString(), tr("Autres / sans contrat")));
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

QWidget *ContractBoardDialog::makeColumn(const QString &contract, const QString &title)
{
    auto *column = new QFrame(this);
    column->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(10, 10, 10, 10);

    auto *header = new QLabel(title, column);
    header->setObjectName(QStringLiteral("title"));
    auto *count = new QLabel(column);
    count->setObjectName(QStringLiteral("muted"));
    m_counts.insert(contract, count);
    layout->addWidget(header);
    layout->addWidget(count);

    auto *list = new ContractList(contract, column);
    m_lists.insert(contract, list);
    connect(list, &ContractList::personDropped, this, &ContractBoardDialog::onPersonDropped);
    layout->addWidget(list, 1);

    auto *add = new QPushButton(tr("+ Ajouter un employé"), column);
    add->setProperty("variant", QStringLiteral("ghost"));
    layout->addWidget(add);
    connect(add, &QPushButton::clicked, this, [this, contract]() { onAddEmployee(contract); });
    return column;
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
        it.value()->clear();
        int count = 0;
        for (const CloudPerson &person : m_employees) {
            if (columnFor(contractOf(person)) != it.key())
                continue;
            auto *item = new QListWidgetItem(QStringLiteral("%1 · %2").arg(person.code, person.name), it.value());
            item->setData(Qt::UserRole, person.id);
            ++count;
        }
        m_counts.value(it.key())->setText(tr("%1 employé(s)").arg(count));
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

void ContractBoardDialog::onPersonDropped(const QString &personId, const QString &targetContract)
{
    const CloudPerson *found = findEmployee(personId);
    if (!found || columnFor(contractOf(*found)) == targetContract)
        return;

    const CloudPerson moved = *found;
    const QString previous = contractOf(moved);
    CloudPerson updated = moved;
    setContract(updated, targetContract);
    if (!m_store.updatePerson(updated)) {
        QMessageBox::warning(this, windowTitle(), tr("Changement impossible : %1").arg(m_store.lastError()));
        return;
    }

    m_undoPersonId = moved.id;
    m_undoPreviousContract = previous;
    m_undoLabel->setText(tr("%1 passe en %2.").arg(moved.name, targetContract.isEmpty() ? tr("Autres") : targetContract));
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
    setContract(restored, m_undoPreviousContract);
    if (!m_store.updatePerson(restored)) {
        QMessageBox::warning(this, windowTitle(), tr("Annulation impossible : %1").arg(m_store.lastError()));
        return;
    }
    m_undoLabel->hide();
    m_undoButton->hide();
    reload();
    emit changed();
}

void ContractBoardDialog::onAddEmployee(const QString &contract)
{
    QStringList codes;
    for (const CloudPerson &person : m_employees)
        codes << person.code;

    PersonDialog dialog(kKindEmployee, nextEmployeeCode(m_employees), codes, this);
    if (!contract.isEmpty()) {
        const int index = contractIndex();
        QStringList preset;
        for (size_t i = 0; i < kindDef(kKindEmployee).fields.size(); ++i)
            preset << QString();
        if (index >= 0)
            preset[index] = contract;
        dialog.setValues(preset);
    }
    if (dialog.exec() != QDialog::Accepted)
        return;

    const PersonRecord created = dialog.person();
    CloudPerson person;
    person.kind = kKindEmployee;
    person.code = created.code;
    person.name = created.name;
    person.fields = created.fields;
    if (!m_store.addPerson(person)) {
        QMessageBox::warning(this, windowTitle(), tr("Ajout impossible : %1").arg(m_store.lastError()));
        return;
    }
    reload();
    emit changed();
}
