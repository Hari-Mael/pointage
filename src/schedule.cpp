#include "schedule.h"

#include <QSettings>

namespace {

QString timeFormat()
{
    return QStringLiteral("HH:mm");
}

QTime readTime(QSettings &settings, const char *key, const QTime &fallback)
{
    const QTime time = QTime::fromString(settings.value(QLatin1String(key)).toString(), timeFormat());
    return time.isValid() ? time : fallback;
}

}

Schedule Schedule::load()
{
    const Schedule defaults;
    Schedule result;
    QSettings settings;
    settings.beginGroup(QStringLiteral("schedule"));
    result.morningStart = readTime(settings, "morningStart", defaults.morningStart);
    result.morningEnd = readTime(settings, "morningEnd", defaults.morningEnd);
    result.pauseStart = readTime(settings, "pauseStart", defaults.pauseStart);
    result.pauseEnd = readTime(settings, "pauseEnd", defaults.pauseEnd);
    result.afternoonStart = readTime(settings, "afternoonStart", defaults.afternoonStart);
    result.afternoonEnd = readTime(settings, "afternoonEnd", defaults.afternoonEnd);
    settings.endGroup();
    return result;
}

void Schedule::save() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("schedule"));
    settings.setValue(QStringLiteral("morningStart"), morningStart.toString(timeFormat()));
    settings.setValue(QStringLiteral("morningEnd"), morningEnd.toString(timeFormat()));
    settings.setValue(QStringLiteral("pauseStart"), pauseStart.toString(timeFormat()));
    settings.setValue(QStringLiteral("pauseEnd"), pauseEnd.toString(timeFormat()));
    settings.setValue(QStringLiteral("afternoonStart"), afternoonStart.toString(timeFormat()));
    settings.setValue(QStringLiteral("afternoonEnd"), afternoonEnd.toString(timeFormat()));
    settings.endGroup();
}

bool Schedule::isValid() const
{
    return morningStart < morningEnd && morningEnd <= pauseStart && pauseStart < pauseEnd
           && pauseEnd <= afternoonStart && afternoonStart < afternoonEnd;
}

QString Schedule::summary() const
{
    return QStringLiteral("matin %1–%2 · pause %3–%4 · après-midi %5–%6")
        .arg(morningStart.toString(timeFormat()), morningEnd.toString(timeFormat()),
             pauseStart.toString(timeFormat()), pauseEnd.toString(timeFormat()),
             afternoonStart.toString(timeFormat()), afternoonEnd.toString(timeFormat()));
}
