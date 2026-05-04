#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "client.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectButtonClicked();
    void onLoginButtonClicked();
    void onAddTaskButtonClicked();

    void onClientConnected();
    void onClientDisconnected();
    void onLoginSuccess(const QString &username);
    void onTaskAdded();
    void onTasksReceived(const QStringList &tasks);
    void onClientErrorOccurred(const QString &message);

private:
    void connectUiSignals();
    void connectClientSignals();
    void setLoggedInUser(const QString &username);

private:
    Ui::MainWindow *ui;
    Client m_client;
    QString m_currentUsername;
};

#endif // MAINWINDOW_H