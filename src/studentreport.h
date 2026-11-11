#pragma once

#include <QDate>
#include <QDialog>
#include <QString>

#include "cloudstore.h"

class QTextBrowser;

namespace StudentReport {

QString toHtml(const CloudPerson &person, const CloudMarks &marks, int year, int month, int restMask,
               const QDate &today, const QString &schoolName);

}

class StudentReportDialog : public QDialog {
    Q_OBJECT
public:
    StudentReportDialog(const CloudPerson &person, const CloudMarks &marks, int year, int month, int restMask,
                        const QDate &today, const QString &schoolName, QWidget *parent = nullptr);

private slots:
    void print();
    void saveAsPdf();

private:
    QTextBrowser *m_view = nullptr;
};
