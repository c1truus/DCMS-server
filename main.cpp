#include <QCoreApplication>
#include "server.h"
#include <QtSql/QSqlDatabase>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
#ifdef DEBUG
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();
#endif
    Server s;
    s.startServer();
    return a.exec();
}
