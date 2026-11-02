#include "logger.h"

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QTextStream>

namespace {

QString g_logPath;
QMutex g_mutex;

const char *levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "DEBUG";
    case QtInfoMsg: return "INFO";
    case QtWarningMsg: return "WARN";
    case QtCriticalMsg: return "ERROR";
    case QtFatalMsg: return "FATAL";
    }
    return "LOG";
}

void handler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    QMutexLocker lock(&g_mutex);
    QFile file(g_logPath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;
    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << levelName(type) << "] "
        << message << '\n';
}

}

namespace Logger {

void install(const QString &logFilePath)
{
    g_logPath = logFilePath;
    qInstallMessageHandler(handler);
}

}
