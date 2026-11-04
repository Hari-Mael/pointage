#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "auth.h"
#include "database.h"
#include "logger.h"
#include "logindialog.h"
#include "mainwindow.h"
#include "secretstore.h"
#include "theme.h"

namespace {

void showLogin(Database &db, const QString &dataDir);

void openMainWindow(Database &db, const QString &dataDir, const UserRecord &user)
{
    qInfo() << "Connexion de" << user.email;
    auto *window = new MainWindow(db, user);
    window->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(window, &MainWindow::logoutRequested, window, [&db, dataDir, window]() {
        qInfo() << "Déconnexion";
        SecretStore::clear(dataDir);
        showLogin(db, dataDir);
        window->close();
    });
    window->show();
}

void showLogin(Database &db, const QString &dataDir)
{
    auto *login = new LoginDialog(db, dataDir);
    if (login->restoreSession()) {
        const UserRecord user = login->user();
        login->deleteLater();
        openMainWindow(db, dataDir, user);
        return;
    }
    QObject::connect(login, &QDialog::accepted, login, [&db, dataDir, login]() {
        openMainWindow(db, dataDir, login->user());
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

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    Logger::install(dataDir + QStringLiteral("/pointage.log"));
    qInfo() << "Démarrage Pointage" << POINTAGE_VERSION;

    Database db;
    QString error;
    if (!db.open(dataDir + QStringLiteral("/pointage.db"), error)) {
        qCritical() << "Ouverture de la base impossible :" << error;
        QMessageBox::critical(nullptr, QStringLiteral("Pointage"),
                              QStringLiteral("Impossible d'ouvrir la base de données : %1").arg(error));
        return 1;
    }

    showLogin(db, dataDir);
    const int code = app.exec();
    qInfo() << "Arrêt Pointage" << code;
    return code;
}
