#include "server.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

Server::Server(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection,
            this, &Server::onNewConnection);
}

bool Server::start(quint16 port)
{
    bool ok = m_server.listen(QHostAddress::Any, port);

    if (ok) {
        qDebug() << "Server listening on port" << port;
    } else {
        qDebug() << "Failed to start server:" << m_server.errorString();
    }

    return ok;
}

void Server::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *clientSocket = m_server.nextPendingConnection();

        qDebug() << "Client connected:"
                 << clientSocket->peerAddress().toString()
                 << clientSocket->peerPort();

        m_buffers[clientSocket] = QByteArray();

        connect(clientSocket, &QTcpSocket::readyRead,
                this, &Server::onReadyRead);

        connect(clientSocket, &QTcpSocket::disconnected,
                this, &Server::onDisconnected);
    }
}

void Server::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());

    if (clientSocket == nullptr) {
        return;
    }

    m_buffers[clientSocket].append(clientSocket->readAll());

    QByteArray &buffer = m_buffers[clientSocket];

    while (buffer.contains('\n')) {
        int newlineIndex = buffer.indexOf('\n');

        QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        qDebug() << "Received from client:" << line;

        processMessage(clientSocket, line);
    }
}

void Server::onDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());

    if (clientSocket == nullptr) {
        return;
    }

    qDebug() << "Client disconnected";

    m_buffers.remove(clientSocket);
    clientSocket->deleteLater();
}

void Server::processMessage(QTcpSocket *clientSocket, const QByteArray &line)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QJsonObject errorObj;
        errorObj["type"] = "error";
        errorObj["message"] = "Invalid JSON";

        sendJson(clientSocket, errorObj);
        return;
    }

    if (!doc.isObject()) {
        QJsonObject errorObj;
        errorObj["type"] = "error";
        errorObj["message"] = "JSON must be an object";

        sendJson(clientSocket, errorObj);
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "login") {
        QString username = obj["username"].toString().trimmed();

        if (username.isEmpty()) {
            QJsonObject errorObj;
            errorObj["type"] = "error";
            errorObj["message"] = "Username is empty";

            sendJson(clientSocket, errorObj);
            return;
        }

        QJsonObject responseObj;
        responseObj["type"] = "login_ok";
        responseObj["username"] = username;

        sendJson(clientSocket, responseObj);
    } else {
        QJsonObject errorObj;
        errorObj["type"] = "error";
        errorObj["message"] = "Unknown request type";

        sendJson(clientSocket, errorObj);
    }
}

void Server::sendJson(QTcpSocket *clientSocket, const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');

    clientSocket->write(data);
}