#include "privacynotice.h"

#include <QMessageBox>

namespace {

const char *const kContactEmail = "contact@exemple.fr";
const char *const kRetention = "la durée de conservation est fixée par l'établissement (par défaut : fin de l'année scolaire suivante)";

}

void showPrivacyNotice(QWidget *parent)
{
    const QString text = QStringLiteral(
        "<h3>Confidentialité des données</h3>"
        "<p><b>Données enregistrées :</b> identité, coordonnées et informations de contact des élèves et des "
        "employés, ainsi que leurs présences et absences. Les élèves sont souvent mineurs : ces données sont "
        "sensibles.</p>"
        "<p><b>Finalité :</b> suivi des présences et absences, uniquement pour l'établissement qui utilise le logiciel.</p>"
        "<p><b>Hébergement :</b> les données sont stockées chez Supabase, sur des serveurs chiffrés. Chaque compte ne "
        "voit que ses propres données.</p>"
        "<p><b>Conservation :</b> %1.</p>"
        "<p><b>Vos droits :</b> accès, rectification, suppression et export de vos données. Pour les exercer, "
        "écrivez à <a href=\"mailto:%2\">%2</a>.</p>"
        "<p><b>Sauvegarde :</b> utilisez le bouton « Exporter CSV » pour conserver une copie des données.</p>")
                             .arg(QString::fromUtf8(kRetention), QString::fromUtf8(kContactEmail));

    QMessageBox box(parent);
    box.setWindowTitle(QStringLiteral("Confidentialité"));
    box.setTextFormat(Qt::RichText);
    box.setText(text);
    box.setIcon(QMessageBox::Information);
    box.exec();
}
