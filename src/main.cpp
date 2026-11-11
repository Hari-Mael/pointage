#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "appstyle.h"
#include "auth.h"
#include "cloudstore.h"
#include "logger.h"
#include "logo.h"
#include "logindialog.h"
#include "mainwindow.h"
#include "secretstore.h"
#include "theme.h"

namespace {

void showLogin(const QString &dataDir);

void openMainWindow(const QString &dataDir, const SupabaseSession &session)
{
    qInfo() << "Connexion de" << session.email;
    auto *store = new CloudStore(session, dataDir);
    const QString name = session.name.isEmpty() ? session.email : session.name;
    auto *window = new MainWindow(*store, name, session.email);
    window->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(window, &QObject::destroyed, [store]() { delete store; });
    QObject::connect(window, &MainWindow::logoutRequested, window, [dataDir, window]() {
        qInfo() << "Déconnexion";
        SecretStore::clear(dataDir);
        showLogin(dataDir);
        window->close();
    });
    window->show();
}

void showLogin(const QString &dataDir)
{
    auto *login = new LoginDialog(dataDir);
    if (login->restoreSession()) {
        const SupabaseSession session = login->session();
        login->deleteLater();
        openMainWindow(dataDir, session);
        return;
    }
    QObject::connect(login, &QDialog::accepted, login, [dataDir, login]() {
        openMainWindow(dataDir, login->session());
    });
    QObject::connect(login, &QDialog::finished, login, &QObject::deleteLater);
    QObject::connect(login, &QDialog::rejected, qApp, &QCoreApplication::quit);
    login->show();
}

}

int main(int argc, char *argv[])
{
    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--selftest")) {
        const bool ok = Auth::selfTest();
        qInfo("Auto-test crypto : %s", ok ? "OK" : "ECHEC");
        return ok ? 0 : 1;
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Pointage"));
    QApplication::setOrganizationName(QStringLiteral("Hari-Mael"));
    QApplication::setApplicationVersion(QStringLiteral(POINTAGE_VERSION));
    applyTheme(app);
    app.setStyleSheet(app.styleSheet() + appStyleSheet());
    app.setWindowIcon(Logo::icon());

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    Logger::install(dataDir + QStringLiteral("/pointage.log"));
    qInfo() << "Démarrage Pointage" << POINTAGE_VERSION;

    showLogin(dataDir);
    const int code = app.exec();
    qInfo() << "Arrêt Pointage" << code;
    return code;
}
