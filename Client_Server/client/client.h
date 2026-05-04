#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QStringList>

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void sendLogin(const QString &username, const QString &password);
    void sendAddTask(const QString &username, const QString &text);
    void sendGetTasks(const QString &username);

signals:
    void connected();
    void disconnected();

    void loginSuccess(const QString &username);
    void taskAdded();
    void tasksReceived(const QStringList &tasks);
    void errorOccurred(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();

private:
    void sendJson(const QJsonObject &obj);

private:
    QTcpSocket m_socket;
    QByteArray m_buffer;
};

#endif // CLIENT_H