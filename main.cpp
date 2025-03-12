#include <QCoreApplication>
#include "server.h"
#include <QtSql/QSqlDatabase>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();
    Server s;
    s.startServer();
    return a.exec();
}
