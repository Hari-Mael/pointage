#include "accountpanel.h"

#include <QFormLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "cloudstore.h"
#include "fields.h"
#include "supabaseauth.h"

namespace {

constexpr int kMinPasswordLength = 8;

}

AccountPanel::AccountPanel(CloudStore &store, const QString &currentEmail, QWidget *parent)
    : QWidget(parent), m_store(store)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto *current = new QLabel(tr("Identifiant actuel : %1").arg(currentEmail), this);
    current->setObjectName(QStringLiteral("muted"));
    current->setWordWrap(true);
    layout->addWidget(current);

    auto *form = new QFormLayout;
    m_email = new QLineEdit(this);
    m_email->setPlaceholderText(tr("Laisser vide pour ne pas changer"));
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(tr("Laisser vide pour ne pas changer"));
    m_confirm = new QLineEdit(this);
    m_confirm->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Nouvel identifiant (email)"), m_email);
    form->addRow(tr("Nouveau mot de passe"), m_password);
    form->addRow(tr("Confirmer le mot de passe"), m_confirm);
    layout->addLayout(form);

    auto *saveButton = new QPushButton(tr("Enregistrer l'identifiant et le mot de passe"), this);
    connect(saveButton, &QPushButton::clicked, this, &AccountPanel::save);
    layout->addWidget(saveButton);
    layout->addStretch(1);
}

void AccountPanel::save()
{
    const QString email = m_email->text().trimmed();
    const QString password = m_password->text();

    if (email.isEmpty() && password.isEmpty()) {
        QMessageBox::information(this, tr("Compte"), tr("Aucune modification à enregistrer."));
        return;
    }
    if (!email.isEmpty() && !isValidEmail(email)) {
        QMessageBox::warning(this, tr("Compte"), tr("Email invalide."));
        return;
    }
    if (!password.isEmpty()) {
        if (password.size() < kMinPasswordLength) {
            QMessageBox::warning(this, tr("Compte"),
                                 tr("Le mot de passe doit contenir au moins %1 caractères.").arg(kMinPasswordLength));
            return;
        }
        if (password != m_confirm->text()) {
            QMessageBox::warning(this, tr("Compte"), tr("Les deux mots de passe ne correspondent pas."));
            return;
        }
    }

    QJsonObject body;
    if (!email.isEmpty())
        body.insert(QStringLiteral("email"), email);
    if (!password.isEmpty())
        body.insert(QStringLiteral("password"), password);

    SupabaseAuth auth;
    QString updatedEmail;
    QString error;
    if (!auth.updateUser(m_store.accessToken(), body, updatedEmail, error)) {
        QMessageBox::warning(this, tr("Compte"), tr("Modification impossible : %1").arg(error));
        return;
    }

    m_email->clear();
    m_password->clear();
    m_confirm->clear();

    QStringList messages;
    if (!password.isEmpty())
        messages << tr("Mot de passe modifié.");
    if (!email.isEmpty())
        messages << tr("Un email de confirmation a été envoyé à %1. Le nouvel identifiant sera actif après validation.")
                        .arg(email);
    QMessageBox::information(this, tr("Compte"), messages.join(QLatin1Char('\n')));
}
