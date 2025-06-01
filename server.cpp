#include "server.h"
#include "debugutils.h"
#include "config.h"

#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QSqlRecord>


Server::Server(QObject *parent) : QTcpServer(parent) {
    QMap<QString, QString> config = loadConfig("/home/ubuntu/Documents/DCMS-versions/server-v0/configs/config.txt");
    QString host = config.value("host", "127.0.0.1");
    QString dbName = config.value("dbname", "DCDB");
    QString dbUser = config.value("dbuser", "admin");
    QString dbPass = config.value("dbpass", "");

    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(host);
    db.setDatabaseName(dbName);
    db.setUserName(dbUser);
    db.setPassword(dbPass);

    if (!db.open()) {
        bich("CRITICAL: Failed DB connection: " + db.lastError().text(), QtCriticalMsg);
    } else {
        bich("Database connected securely.", QtDebugMsg);
    }
}

void Server::startServer() {
    if (!this->listen(QHostAddress::Any, 12345)) {
        bich("Server could not start! " + this->errorString(), QtCriticalMsg);
    } else {
        bich("Server started on port 12345", QtDebugMsg);
        for (const QHostAddress &ip : QNetworkInterface::allAddresses()) {
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip != QHostAddress::LocalHost) {
                bich("Server IP address: " + ip.toString(), QtDebugMsg);
            }
        }
    }
}

void Server::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *clientSocket = new QTcpSocket(this);
    if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
        bich("Failed to set socket descriptor", QtCriticalMsg);
        clientSocket->deleteLater();
        return;
    }

    connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
        QByteArray data = clientSocket->readAll();
        handleClientCommand(clientSocket, data);
    });

    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);

    bich("New client connected", QtDebugMsg);
}

void Server::handleClientCommand(QTcpSocket* socket, const QByteArray& rawData) {
    QList<QByteArray> parts = rawData.split(',');

    if (parts.size() < 2) {
        sendError(socket, "Malformed request");
        return;
    }

    QString command = parts[0].trimmed();
    QString username = parts[1].trimmed();

    if (command == "LOGIN" && parts.size() == 3) {
        handleLogin(socket, username, parts[2].trimmed());
    } else if (command == "FETCH" && parts.size() == 3) {
        handleFetch(socket, username, parts[2].trimmed());
    } else {
        sendError(socket, "Invalid command");
    }
}

void Server::handleLogin(QTcpSocket* socket, const QString& username, const QString& password) {
    if (validateCredentials(username, password)) {
        QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
        activeSessions[username] = qMakePair(token, QDateTime::currentDateTime());
        removeExpiredTokens();

        QJsonObject response{
            {"status", "success"},
            {"user", username},
            {"token", token}
        };
        sendJson(socket, response);
        bich(username + " logged in with session token: " + token, QtDebugMsg);
    } else {
        sendError(socket, "Invalid Credentials");
    }
}

void Server::handleFetch(QTcpSocket* socket, const QString& username, const QString& token) {
    if (!validateSession(token)) {
        sendError(socket, "Invalid or expired session token");
        return;
    }

    QStringList data = queryDentistInfo(username);

    if (data.size() == 1 && data[0] == "error1") {
        sendError(socket, "Database query failed");
    } else if (data.size() == 1 && data[0] == "error2") {
        sendError(socket, "No data found for user");
    } else {
        QJsonArray infoArray;
        for (const QString& field : data) {
            infoArray.append(field);
        }

        QJsonObject response{
            {"status", "success"},
            {"dentist_info", infoArray}
        };
        sendJson(socket, response);
    }
}

void Server::sendError(QTcpSocket* socket, const QString& message) {
    QJsonObject response{
        {"status", "error"},
        {"message", message}
    };
    sendJson(socket, response);
}

void Server::sendJson(QTcpSocket* socket, const QJsonObject& obj) {
    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
    socket->flush();
}

QStringList Server::queryDentistInfo(const QString &user) {
    QStringList dentistInfoList;

    QSqlQuery query;
    query.prepare("SELECT * FROM dentists "
                  "INNER JOIN auth_users ON dentists.user_id = auth_users.id "
                  "WHERE auth_users.username = :user");
    query.bindValue(":user", user);

    if (!query.exec()) {
        bich("Query failed: " + query.lastError().text(), QtCriticalMsg);
        dentistInfoList.append("error1");
        return dentistInfoList;
    }

    if (query.next()) {
        for (int i = 0; i < query.record().count(); ++i) {
            dentistInfoList.append(query.value(i).toString());
        }
        return dentistInfoList;
    }

    dentistInfoList.append("error2");
    return dentistInfoList;
}

bool Server::validateCredentials(const QString &username, const QString &password) {
    QSqlQuery query;
    query.prepare("SELECT password_hash FROM auth_users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        bich("Query failed: " + query.lastError().text(), QtCriticalMsg);
        return false;
    }

    if (query.next()) {
        QString storedHash = query.value(0).toString();
        QString inputHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        return storedHash == inputHash;
    }

    return false;
}

bool Server::validateSession(const QString &token) {
    const QDateTime now = QDateTime::currentDateTime();

    for (auto it = activeSessions.begin(); it != activeSessions.end(); ++it) {
        if (it.value().first == token) {
            // Token found
            if (it.value().second.secsTo(now) <= 3600) {
                return true;
            }
        }
    }
    return false;
}

void Server::removeExpiredTokens() {
    const QDateTime now = QDateTime::currentDateTime();

    for (auto it = activeSessions.begin(); it != activeSessions.end(); ) {
        if (it.value().second.secsTo(now) > 3600) {
            it = activeSessions.erase(it);
        } else {
            ++it;
        }
    }
}
