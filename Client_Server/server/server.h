#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QStringList>
#include <QJsonObject>

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);

    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void processMessage(QTcpSocket *clientSocket, const QByteArray &line);
    void sendJson(QTcpSocket *clientSocket, const QJsonObject &obj);

    bool loadUsers();
    bool loadTasks();
    bool saveTasks();

private:
    QTcpServer m_server;
    QHash<QTcpSocket*, QByteArray> m_buffers;

    QHash<QString, QString> m_users;
    QHash<QString, QStringList> m_tasksByUser;

    QString m_usersFilePath;
    QString m_tasksFilePath;
};

#endif // SERVER_H