#pragma once

#include <QDialog>
#include <QListWidget>
#include <QMap>
#include <vector>

#include "cloudstore.h"

class QLabel;
class QPushButton;
class QVBoxLayout;

class ClassList : public QListWidget {
    Q_OBJECT
public:
    explicit ClassList(const QString &className, QWidget *parent = nullptr);
    QString className() const { return m_className; }

signals:
    void personDropped(const QString &personId, const QString &targetClass);

protected:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool accepts(const QMimeData *data, const QObject *source) const;

    QString m_className;
};

class ClassBoardDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClassBoardDialog(CloudStore &store, QWidget *parent = nullptr);

signals:
    void changed();

private:
    void reload();
    void onPersonDropped(const QString &personId, const QString &targetClass);
    void onAddStudent(const QString &className);
    void undoLastMove();
    const CloudPerson *findStudent(const QString &personId) const;
    QWidget *makeColumn(const QString &className);

    CloudStore &m_store;
    std::vector<CloudPerson> m_students;
    QMap<QString, ClassList *> m_lists;
    QMap<QString, QLabel *> m_counts;
    QVBoxLayout *m_columnsLayout = nullptr;
    QLabel *m_undoLabel = nullptr;
    QPushButton *m_undoButton = nullptr;
    QString m_undoPersonId;
    QString m_undoPreviousClass;
};
