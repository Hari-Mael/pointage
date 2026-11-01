#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "auth.h"
#include "database.h"
#include "logindialog.h"
#include "mainwindow.h"

namespace {

void showLogin(Database &db);

void openMainWindow(Database &db, const UserRecord &user)
{
    auto *window = new MainWindow(db, user);
    window->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(window, &MainWindow::logoutRequested, window, [&db, window]() {
        showLogin(db);
        window->close();
    });
    window->show();
}

void showLogin(Database &db)
{
    auto *login = new LoginDialog(db);
    QObject::connect(login, &QDialog::accepted, login, [&db, login]() {
        openMainWindow(db, login->user());
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

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    Database db;
    QString error;
    if (!db.open(dataDir + QStringLiteral("/pointage.db"), error)) {
        QMessageBox::critical(nullptr, QStringLiteral("Pointage"),
                              QStringLiteral("Impossible d'ouvrir la base de données : %1").arg(error));
        return 1;
    }

    showLogin(db);
    return app.exec();
}
