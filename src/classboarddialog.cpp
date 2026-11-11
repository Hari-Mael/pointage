#include "classboarddialog.h"

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
#include "schoolclasses.h"

namespace {

const char *const kPersonMime = "application/x-pointage-person";
const char *const kUnassigned = "";

QString nextCodeIn(const std::vector<CloudPerson> &people, const QString &className)
{
    int highest = 0;
    for (const CloudPerson &person : people) {
        if (person.kind != kKindStudent || person.className != className)
            continue;
        bool ok = false;
        const int number = person.code.mid(1).toInt(&ok);
        if (ok && number > highest)
            highest = number;
    }
    const QChar prefix = kindDef(kKindStudent).prefix;
    return QStringLiteral("%1%2").arg(prefix).arg(highest + 1, 3, 10, QLatin1Char('0'));
}

}

ClassList::ClassList(const QString &className, QWidget *parent)
    : QListWidget(parent), m_className(className)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::CopyAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setMinimumWidth(190);
}

bool ClassList::accepts(const QMimeData *data, const QObject *source) const
{
    return data && data->hasFormat(QLatin1String(kPersonMime)) && source != this;
}

QStringList ClassList::mimeTypes() const
{
    return {QLatin1String(kPersonMime)};
}

QMimeData *ClassList::mimeData(const QList<QListWidgetItem *> &items) const
{
    auto *data = new QMimeData;
    if (!items.isEmpty())
        data->setData(QLatin1String(kPersonMime), items.first()->data(Qt::UserRole).toString().toUtf8());
    return data;
}

void ClassList::dragEnterEvent(QDragEnterEvent *event)
{
    if (accepts(event->mimeData(), event->source()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ClassList::dragMoveEvent(QDragMoveEvent *event)
{
    if (accepts(event->mimeData(), event->source()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ClassList::dropEvent(QDropEvent *event)
{
    if (!accepts(event->mimeData(), event->source())) {
        event->ignore();
        return;
    }
    const QString personId = QString::fromUtf8(event->mimeData()->data(QLatin1String(kPersonMime)));
    event->setDropAction(Qt::CopyAction);
    event->accept();
    emit personDropped(personId, m_className);
}

ClassBoardDialog::ClassBoardDialog(CloudStore &store, QWidget *parent)
    : QDialog(parent), m_store(store)
{
    setWindowTitle(tr("Élèves par classe"));
    resize(1100, 640);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *hint = new QLabel(tr("Glissez un élève d'une classe vers une autre pour le déplacer. "
                               "Le matricule est unique dans chaque classe."),
                            this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget(scroll);
    m_columnsLayout = new QVBoxLayout(content);
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    for (const QString &className : SchoolClasses::all())
        row->addWidget(makeColumn(className));
    row->addWidget(makeColumn(QString::fromLatin1(kUnassigned)));
    row->addStretch(1);
    m_columnsLayout->addLayout(row);
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

    connect(m_undoButton, &QPushButton::clicked, this, &ClassBoardDialog::undoLastMove);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    reload();
}

QWidget *ClassBoardDialog::makeColumn(const QString &className)
{
    auto *column = new QFrame(this);
    column->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(10, 10, 10, 10);

    const QString title = className.isEmpty() ? tr("Sans classe") : className;
    auto *header = new QLabel(title, column);
    header->setObjectName(QStringLiteral("title"));
    auto *count = new QLabel(column);
    count->setObjectName(QStringLiteral("muted"));
    m_counts.insert(className, count);
    layout->addWidget(header);
    layout->addWidget(count);

    auto *list = new ClassList(className, column);
    m_lists.insert(className, list);
    connect(list, &ClassList::personDropped, this, &ClassBoardDialog::onPersonDropped);
    layout->addWidget(list, 1);

    auto *add = new QPushButton(tr("+ Ajouter un élève"), column);
    add->setProperty("variant", QStringLiteral("ghost"));
    layout->addWidget(add);
    connect(add, &QPushButton::clicked, this, [this, className]() { onAddStudent(className); });
    return column;
}

void ClassBoardDialog::reload()
{
    m_students.clear();
    std::vector<CloudPerson> all;
    if (!m_store.fetchPeople(all)) {
        QMessageBox::warning(this, windowTitle(), tr("Lecture des données impossible : %1").arg(m_store.lastError()));
        return;
    }
    for (const CloudPerson &person : all) {
        if (person.kind == kKindStudent)
            m_students.push_back(person);
    }

    for (auto it = m_lists.begin(); it != m_lists.end(); ++it) {
        it.value()->clear();
        int count = 0;
        for (const CloudPerson &person : m_students) {
            if (person.className != it.key())
                continue;
            auto *item = new QListWidgetItem(QStringLiteral("%1 · %2").arg(person.code, person.name), it.value());
            item->setData(Qt::UserRole, person.id);
            ++count;
        }
        m_counts.value(it.key())->setText(tr("%1 élève(s)").arg(count));
    }
}

const CloudPerson *ClassBoardDialog::findStudent(const QString &personId) const
{
    for (const CloudPerson &person : m_students) {
        if (person.id == personId)
            return &person;
    }
    return nullptr;
}

void ClassBoardDialog::onPersonDropped(const QString &personId, const QString &targetClass)
{
    const CloudPerson *found = findStudent(personId);
    if (!found || found->className == targetClass)
        return;

    const CloudPerson moved = *found;
    for (const CloudPerson &other : m_students) {
        if (other.className == targetClass && other.code == moved.code) {
            QMessageBox::warning(this, windowTitle(),
                                 tr("Le matricule %1 existe déjà dans la classe %2. Modifiez la fiche avant de déplacer l'élève.")
                                     .arg(moved.code, targetClass.isEmpty() ? tr("Sans classe") : targetClass));
            return;
        }
    }

    CloudPerson updated = moved;
    updated.className = targetClass;
    if (!m_store.updatePerson(updated)) {
        QMessageBox::warning(this, windowTitle(), tr("Déplacement impossible : %1").arg(m_store.lastError()));
        return;
    }

    m_undoPersonId = moved.id;
    m_undoPreviousClass = moved.className;
    m_undoLabel->setText(tr("%1 déplacé vers %2.").arg(moved.name, targetClass.isEmpty() ? tr("Sans classe") : targetClass));
    m_undoLabel->show();
    m_undoButton->show();
    reload();
    emit changed();
}

void ClassBoardDialog::undoLastMove()
{
    const CloudPerson *found = findStudent(m_undoPersonId);
    if (!found)
        return;
    CloudPerson restored = *found;
    restored.className = m_undoPreviousClass;
    if (!m_store.updatePerson(restored)) {
        QMessageBox::warning(this, windowTitle(), tr("Annulation impossible : %1").arg(m_store.lastError()));
        return;
    }
    m_undoLabel->hide();
    m_undoButton->hide();
    reload();
    emit changed();
}

void ClassBoardDialog::onAddStudent(const QString &className)
{
    QStringList codes;
    for (const CloudPerson &person : m_students) {
        if (person.className == className)
            codes << person.code;
    }

    PersonDialog dialog(kKindStudent, nextCodeIn(m_students, className), codes, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const PersonRecord created = dialog.person();
    CloudPerson person;
    person.kind = kKindStudent;
    person.className = className;
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
