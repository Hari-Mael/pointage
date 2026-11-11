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
    explicit ContractList(const QString &contract, QWidget *parent = nullptr);
    QString contract() const { return m_contract; }

signals:
    void personDropped(const QString &personId, const QString &targetContract);

protected:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool accepts(const QMimeData *data, const QObject *source) const;

    QString m_contract;
};

class ContractBoardDialog : public QDialog {
    Q_OBJECT
public:
    explicit ContractBoardDialog(CloudStore &store, QWidget *parent = nullptr);

signals:
    void changed();

private:
    QWidget *makeColumn(const QString &contract, const QString &title);
    void reload();
    void onPersonDropped(const QString &personId, const QString &targetContract);
    void onAddEmployee(const QString &contract);
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
