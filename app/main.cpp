#include "Environment.h"
#include <QApplication>
#include <QLabel>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    if (!sqliteDriverAvailable()) {
        QMessageBox::critical(nullptr, QObject::tr("Грешка"),
                              QObject::tr("Драйверът QSQLITE не е наличен."));
        return 1;
    }

    QLabel window(QObject::tr("Складова система — начална настройка"));
    window.setWindowTitle(QObject::tr("Складова система"));
    window.resize(480, 160);
    window.show();
    return application.exec();
}
