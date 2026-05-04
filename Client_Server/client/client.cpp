#include "client.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QAbstractSocket>

Client::Client(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::connected,
            this, &Client::onConnected);

    connect(&m_socket, &QTcpSocket::disconnected,
            this, &Client::onDisconnected);

    connect(&m_socket, &QTcpSocket::readyRead,
            this, &Client::onReadyRead);

    connect(&m_socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                emit errorOccurred(m_socket.errorString());
            });
}

void Client::connectToServer(const QString &host, quint16 port)
{
    m_socket.connectToHost(host, port);
}

void Client::sendLogin(const QString &username, const QString &password)
{
    QJsonObject obj;
    obj["type"] = "login";
    obj["username"] = username;
    obj["password"] = password;

    sendJson(obj);
}

void Client::sendAddTask(const QString &username, const QString &text)
{
    QJsonObject obj;
    obj["type"] = "add_task";
    obj["username"] = username;
    obj["text"] = text;

    sendJson(obj);
}

void Client::sendGetTasks(const QString &username)
{
    QJsonObject obj;
    obj["type"] = "get_tasks";
    obj["username"] = username;

    sendJson(obj);
}

void Client::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');

    m_socket.write(data);
}

void Client::onConnected()
{
    emit connected();
}

void Client::onDisconnected()
{
    emit disconnected();
}

void Client::onReadyRead()
{
    m_buffer.append(m_socket.readAll());

    while (m_buffer.contains('\n')) {
        int newlineIndex = m_buffer.indexOf('\n');
        QByteArray line = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit errorOccurred("Invalid JSON received from server");
            continue;
        }

        if (!doc.isObject()) {
            emit errorOccurred("Server response is not a JSON object");
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();

        if (type == "login_ok") {
            QString username = obj["username"].toString();
            emit loginSuccess(username);
        }
        else if (type == "add_task_ok") {
            emit taskAdded();
        }
        else if (type == "tasks") {
            QStringList tasks;
            QJsonArray items = obj["items"].toArray();

            for (const QJsonValue &value : items) {
                tasks.append(value.toString());
            }

            emit tasksReceived(tasks);
        }
        else if (type == "error") {
            QString message = obj["message"].toString("Unknown server error");
            emit errorOccurred(message);
        }
    }
}