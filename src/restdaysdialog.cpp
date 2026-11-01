#include "restdaysdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

RestDaysDialog::RestDaysDialog(int mask, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Jours de repos"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Cochez les jours non travaillés (grisés dans la feuille) :"), this));

    const QLocale french(QLocale::French);
    for (int dayOfWeek = 1; dayOfWeek <= 7; ++dayOfWeek) {
        const int bit = dayOfWeek % 7;
        auto *box = new QCheckBox(french.dayName(dayOfWeek, QLocale::LongFormat), this);
        box->setChecked((mask >> bit) & 1);
        layout->addWidget(box);
        m_days.push_back({bit, box});
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

int RestDaysDialog::mask() const
{
    int result = 0;
    for (const auto &[bit, box] : m_days) {
        if (box->isChecked())
            result |= 1 << bit;
    }
    return result;
}
