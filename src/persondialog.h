#pragma once

#include <QDialog>
#include <QStringList>
#include <vector>

#include "database.h"

class QWidget;

class PersonDialog : public QDialog {
    Q_OBJECT
public:
    PersonDialog(int kind, const QString &suggestedCode, const QStringList &existingCodes, QWidget *parent = nullptr);
    PersonRecord person() const { return m_person; }
    void setEditMode();
    void setValues(const QStringList &values);

private slots:
    void validateAndAccept();

private:
    int m_kind;
    QStringList m_existingCodes;
    class QLineEdit *m_code = nullptr;
    std::vector<QWidget *> m_inputs;
    PersonRecord m_person;
};
