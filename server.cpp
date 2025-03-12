#include "server.h"
#include <QDebug>
#include <QNetworkInterface>


Server::Server(QObject *parent) : QTcpServer(parent) {
    // Connect to MySQL database
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("10.194.50.223"); //BIT
    // db.setHostName("10.7.22.194"); ///PKU
    // db.setHostName("192.168.1.96"); //USTB
    db.setDatabaseName("DCDB");
    db.setUserName("admin");
    db.setPassword("opensesame42!");

    if (!db.open()) {
        qCritical() << "Failed to connect to database:" << db.lastError().text();
    }
    else{
        qDebug() << "Ok... We are in.";
    }
}

void Server::startServer() {
    if (!this->listen(QHostAddress::Any, 12345)) {
        qCritical() << "Server could not start!";
    } else {
        qDebug() << "Server started on port" << this->serverPort();

        // Get and print all possible IP addresses of the server
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (const QHostAddress &ip : ipAddressesList) {
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip != QHostAddress::LocalHost) {
                qDebug() << "Server tokenIP Address:" << ip.toString();
            }
        }
    }
}

void Server::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *clientSocket = new QTcpSocket();
    clientSocket->setSocketDescriptor(socketDescriptor);

    connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
        QByteArray data = clientSocket->readAll();
        QList<QByteArray> requestData = data.split(',');

        if (requestData.size() >= 2) {
            QString command = requestData[0].trimmed();
            QString username = requestData[1].trimmed();

            if (command == "LOGIN" && requestData.size() == 3) {
                // Login request with username and password
                QString password = requestData[2].trimmed();
                if (validateCredentials(username, password)) {
                    // Generate a new session token
                    QString sessionToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    QDateTime currentTime = QDateTime::currentDateTime();

                    // Store session token and timestamp
                    activeSessions[username] = qMakePair(sessionToken, currentTime);

                    // Clean expired tokens
                    removeExpiredTokens();

                    // Respond with session token
                    QString response = QString("{\"status\":\"success\",\"user\":\"%1\", \"token\":\"%2\"}\n")
                                           .arg(username, sessionToken);
                    clientSocket->write(response.toUtf8());
                    qDebug() << username + " logged in with session token: " + sessionToken;
                } else {
                    clientSocket->write("{\"status\":\"error\", \"message\":\"Invalid Credentials\"}\n");
                }
            }
            else if (command == "FETCH" && requestData.size() == 3) {
                // Fetch dentist info request (authenticated request)
                QString sessionToken = requestData[2].trimmed();

                if (validateSession(sessionToken)) {
                    // Fetch the dentist's info from the database (or wherever)
                    QString dentistInfoStr;
                    QStringList dentistInfoList = queryDentistInfo(username);

                    if (dentistInfoList.size() == 1 && dentistInfoList[0] == "error1") {
                        dentistInfoStr = "{\"status\":\"error\", \"message\":\"Database query failed\"}";
                    } else if (dentistInfoList.size() == 1 && dentistInfoList[0] == "error2") {
                        dentistInfoStr = "{\"status\":\"error\", \"message\":\"No data found for user\"}";
                    } else {
                        // Here we're assuming dentistInfoList has multiple fields to send
                        dentistInfoStr = "{\"status\":\"success\",\"dentist_info\":[";
                        for (int i = 0; i < dentistInfoList.size(); ++i) {
                            dentistInfoStr += "\"" + dentistInfoList[i] + "\"";
                            if (i < dentistInfoList.size() - 1) {
                                dentistInfoStr += ",";
                            }
                        }
                        dentistInfoStr += "]}";
                    }

                    clientSocket->write(dentistInfoStr.toUtf8());
                } else {
                    clientSocket->write("{\"status\":\"error\", \"message\":\"Invalid or expired session token\"}\n");
                }
            }
            else {
                clientSocket->write("{\"status\":\"error\", \"message\":\"Invalid command\"}\n");
            }
        } else {
            clientSocket->write("{\"status\":\"error\", \"message\":\"Malformed request\"}\n");
        }

        clientSocket->flush();
        // Do not disconnect the socket here yet! Allow further requests.
    });

    qDebug() << "New client connected.";
}

QStringList Server::queryDentistInfo(const QString &user) {
    QStringList dentistInfoList;

    QSqlQuery query;
    query.prepare("SELECT * FROM dentists "
                  "INNER JOIN auth_users ON dentists.user_id = auth_users.id "
                  "WHERE auth_users.username = :user");
    query.bindValue(":user", user);

    if (!query.exec()) {
        qCritical() << "Query failed:" << query.lastError().text();
        dentistInfoList.append("error1");
        return dentistInfoList;
    }

    if (query.next()) {
        dentistInfoList.append(query.value(0).toString()); // Dentist Id
        dentistInfoList.append(query.value(1).toString()); // Dentist UserId
        dentistInfoList.append(query.value(2).toString()); // Dentist Name
        dentistInfoList.append(query.value(3).toString()); // Dentist Birthdate
        dentistInfoList.append(query.value(4).toString()); // Dentist Gender
        dentistInfoList.append(query.value(5).toString()); // Dentist Phone
        dentistInfoList.append(query.value(6).toString()); // Dentist Email
        dentistInfoList.append(query.value(7).toString()); // Dentist Role
        dentistInfoList.append(query.value(8).toString()); // Dentist Created@
        // Add more fields as needed
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
        qCritical() << "Query failed:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        QString storedHash = query.value(0).toString();
        QString inputHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        qDebug() << "Validation query done!";
        return storedHash == inputHash;
    }

    return false;
}

bool Server::validateSession(const QString &token) {
    for (auto it = activeSessions.begin(); it != activeSessions.end(); ++it) {
        if (it.value().first == token) {
            QDateTime storedTime = it.value().second;
            if (storedTime.secsTo(QDateTime::currentDateTime()) > 12 * 3600) { // 12 hours = 43200 sec
                activeSessions.erase(it);
                qDebug() << "Session token expired";
                return false; // Token expired
            }
            return true; // Token is valid
        }
    }
    return false; // Token not found
}


void Server::removeExpiredTokens() {
    QDateTime now = QDateTime::currentDateTime();

    for (auto it = activeSessions.begin(); it != activeSessions.end(); ) {
        if (it.value().second.secsTo(now) > 12 * 3600) {
            qDebug() << "Session expired for user:" << it.key();
            it = activeSessions.erase(it); // Remove expired session
        } else {
            ++it;
        }
    }
}
