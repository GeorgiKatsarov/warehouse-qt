#include "Environment.h"
#include <QSqlDatabase>

bool sqliteDriverAvailable()
{
    return QSqlDatabase::isDriverAvailable("QSQLITE");
}
