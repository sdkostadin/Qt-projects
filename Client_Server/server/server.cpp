#include "server.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>

Server::Server(QObject *parent)
    : QObject(parent)
    , m_usersFilePath("users.json")
    , m_tasksFilePath("tasks.json")
{
    connect(&m_server, &QTcpServer::newConnection,
            this, &Server::onNewConnection);

    if (!loadUsers()) {
        qDebug() << "Failed to load users.json";
    }

    if (!loadTasks()) {
        qDebug() << "Failed to load tasks.json";
    }
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

bool Server::loadUsers()
{
    m_users.clear();

    QFile file(m_usersFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open users file:" << m_usersFilePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "users.json parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isArray()) {
        qDebug() << "users.json must contain a JSON array";
        return false;
    }

    QJsonArray array = doc.array();

    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }

        QJsonObject obj = value.toObject();
        QString username = obj["username"].toString().trimmed();
        QString password = obj["password"].toString();

        if (username.isEmpty() || password.isEmpty()) {
            continue;
        }

        m_users[username] = password;
    }

    qDebug() << "Loaded users:" << m_users.keys();
    return true;
}

bool Server::loadTasks()
{
    m_tasksByUser.clear();

    QFile file(m_tasksFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open tasks file:" << m_tasksFilePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "tasks.json parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qDebug() << "tasks.json must contain a JSON object";
        return false;
    }

    QJsonObject root = doc.object();

    for (auto it = root.begin(); it != root.end(); ++it) {
        QString username = it.key();

        if (!it.value().isArray()) {
            continue;
        }

        QJsonArray tasksArray = it.value().toArray();
        QStringList tasks;

        for (const QJsonValue &taskValue : tasksArray) {
            tasks.append(taskValue.toString());
        }

        m_tasksByUser[username] = tasks;
    }

    qDebug() << "Loaded tasks for users:" << m_tasksByUser.keys();
    return true;
}

bool Server::saveTasks()
{
    QJsonObject root;

    for (auto it = m_tasksByUser.begin(); it != m_tasksByUser.end(); ++it) {
        QJsonArray tasksArray;

        for (const QString &task : it.value()) {
            tasksArray.append(task);
        }

        root[it.key()] = tasksArray;
    }

    QJsonDocument doc(root);

    QFile file(m_tasksFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot write tasks file:" << m_tasksFilePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
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

        qDebug() << "Received:" << line;
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
        QString password = obj["password"].toString();

        if (!m_users.contains(username)) {
            QJsonObject errorObj;
            errorObj["type"] = "error";
            errorObj["message"] = "Unknown user";
            sendJson(clientSocket, errorObj);
            return;
        }

        if (m_users.value(username) != password) {
            QJsonObject errorObj;
            errorObj["type"] = "error";
            errorObj["message"] = "Invalid password";
            sendJson(clientSocket, errorObj);
            return;
        }

        QJsonObject responseObj;
        responseObj["type"] = "login_ok";
        responseObj["username"] = username;
        sendJson(clientSocket, responseObj);
    }
    else if (type == "add_task") {
        QString username = obj["username"].toString().trimmed();
        QString text = obj["text"].toString().trimmed();

        if (username.isEmpty() || text.isEmpty()) {
            QJsonObject errorObj;
            errorObj["type"] = "error";
            errorObj["message"] = "Invalid add_task request";
            sendJson(clientSocket, errorObj);
            return;
        }

        if (!m_users.contains(username)) {
            QJsonObject errorObj;
            errorObj["type"] = "error";
            errorObj["message"] = "Unknown user";
            sendJson(clientSocket, errorObj);
            return;
        }

        m_tasksByUser[username].append(text);
        saveTasks();

        QJsonObject responseObj;
        responseObj["type"] = "add_task_ok";
        sendJson(clientSocket, responseObj);
    }
    else if (type == "get_tasks") {
        QString username = obj["username"].toString().trimmed();

        if (!m_users.contains(username)) {
            QJsonObject errorObj;
            errorObj["type"] = "error";
            errorObj["message"] = "Unknown user";
            sendJson(clientSocket, errorObj);
            return;
        }

        QJsonArray items;
        const QStringList tasks = m_tasksByUser.value(username);

        for (const QString &task : tasks) {
            items.append(task);
        }

        QJsonObject responseObj;
        responseObj["type"] = "tasks";
        responseObj["items"] = items;
        sendJson(clientSocket, responseObj);
    }
    else {
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