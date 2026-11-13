#pragma once

#include <QDialog>
#include <QListWidget>
#include <QMap>
#include <vector>

#include "cloudstore.h"

class QLabel;
class QPushButton;

class ContractList : public QListWidget {
    Q_OBJECT
public:
    explicit ContractList(const QString &column, QWidget *parent = nullptr);
    QString column() const { return m_column; }

signals:
    void personDropped(const QString &personId, const QString &targetColumn);

protected:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool accepts(const QMimeData *data, const QObject *source) const;

    QString m_column;
};

class ContractBoardDialog : public QDialog {
    Q_OBJECT
public:
    explicit ContractBoardDialog(CloudStore &store, QWidget *parent = nullptr);

signals:
    void changed();

private:
    QWidget *makeColumn(const QString &column);
    void reload();
    void onPersonDropped(const QString &personId, const QString &targetColumn);
    void onAddEmployee(const QString &column);
    void undoLastMove();
    const CloudPerson *findEmployee(const QString &personId) const;

    CloudStore &m_store;
    std::vector<CloudPerson> m_employees;
    QMap<QString, ContractList *> m_lists;
    QMap<QString, QLabel *> m_counts;
    QLabel *m_undoLabel = nullptr;
    QPushButton *m_undoButton = nullptr;
    QString m_undoPersonId;
    QString m_undoPreviousContract;
};
