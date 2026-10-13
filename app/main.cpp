#include "Database.h"
#include "WarehouseService.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication application(argc,argv);
    QCoreApplication::setOrganizationName("University");
    QCoreApplication::setApplicationName("WarehouseQt");
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QDir().mkpath(directory)) {
        QMessageBox::critical(nullptr,QObject::tr("Грешка"),QObject::tr("Не може да се създаде папката за данни."));
        return 1;
    }
    Database database(directory + "/warehouse.sqlite");
    const QString error = database.open();
    if (!error.isEmpty()) { QMessageBox::critical(nullptr,QObject::tr("Грешка"),error); return 1; }
    WarehouseService service(database.connection());
    LoginDialog login(&service,nullptr);
    if (login.exec() != QDialog::Accepted) return 0;
    MainWindow window(&service,nullptr); window.show();
    return application.exec();
}
