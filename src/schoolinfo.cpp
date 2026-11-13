#include "schoolinfo.h"

#include <QSettings>

namespace SchoolInfo {

QString name()
{
    QSettings settings;
    const QString stored = settings.value(QStringLiteral("school/name")).toString().trimmed();
    return stored.isEmpty() ? QStringLiteral("ECOLE SACRE COEUR") : stored;
}

}
