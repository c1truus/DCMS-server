#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QUuid>
#include <QMap>
#include <QDateTime>

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;
    void handleRequest(qintptr socketDescriptor);
private:
    QSqlDatabase db;
    QMap<QString, QPair<QString, QDateTime>> activeSessions; // { username : {token, timestamp} }
    bool validateCredentials(const QString &username, const QString &password);
    bool validateSession(const QString &token);
    void removeExpiredTokens();
    QStringList queryDentistInfo(const QString &user);
};

#endif // SERVER_H
