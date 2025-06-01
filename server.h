#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <QDateTime>
#include <QUuid>

class Server : public QTcpServer {
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QSqlDatabase db;
    QMap<QString, QPair<QString, QDateTime>> activeSessions;

    void handleClient(QTcpSocket* socket);
    void handleClientCommand(QTcpSocket* socket, const QByteArray& data);
    void handleLogin(QTcpSocket* socket, const QString& username, const QString& password);
    void handleFetch(QTcpSocket* socket, const QString& username, const QString& token);

    void sendError(QTcpSocket* socket, const QString& message);
    void sendJson(QTcpSocket* socket, const QJsonObject& obj);

    QStringList queryDentistInfo(const QString& user);
    bool validateCredentials(const QString& username, const QString& password);
    bool validateSession(const QString& token);
    void removeExpiredTokens();
};

#endif // SERVER_H
