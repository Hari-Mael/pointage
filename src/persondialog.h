#pragma once

#include <QDialog>
#include <QStringList>
#include <vector>

#include "database.h"

class QLabel;
class QLineEdit;
class QWidget;

class PersonDialog : public QDialog {
    Q_OBJECT
public:
    PersonDialog(int kind, const QString &suggestedCode, const QStringList &existingCodes, QWidget *parent = nullptr);
    PersonRecord person() const { return m_person; }
    QString photo() const { return m_photo; }
    void setPhoto(const QString &encoded);
    void setEditMode();
    void setValues(const QStringList &values);

private slots:
    void validateAndAccept();
    void choosePhoto();
    void removePhoto();
    void refreshPreview();

private:
    int m_kind;
    QStringList m_existingCodes;
    QLineEdit *m_code = nullptr;
    QLabel *m_preview = nullptr;
    std::vector<QWidget *> m_inputs;
    PersonRecord m_person;
    QString m_photo;
};
