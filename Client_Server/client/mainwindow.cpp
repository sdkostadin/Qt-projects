#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connectUiSignals();
    connectClientSignals();

    ui->statusLabel->setText("Disconnected");
    ui->currentUserLabel->setText("Current user: none");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectUiSignals()
{
    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectButtonClicked);

    connect(ui->loginButton, &QPushButton::clicked,
            this, &MainWindow::onLoginButtonClicked);

    connect(ui->addTaskButton, &QPushButton::clicked,
            this, &MainWindow::onAddTaskButtonClicked);
}

void MainWindow::connectClientSignals()
{
    connect(&m_client, &Client::connected,
            this, &MainWindow::onClientConnected);

    connect(&m_client, &Client::disconnected,
            this, &MainWindow::onClientDisconnected);

    connect(&m_client, &Client::loginSuccess,
            this, &MainWindow::onLoginSuccess);

    connect(&m_client, &Client::taskAdded,
            this, &MainWindow::onTaskAdded);

    connect(&m_client, &Client::tasksReceived,
            this, &MainWindow::onTasksReceived);

    connect(&m_client, &Client::errorOccurred,
            this, &MainWindow::onClientErrorOccurred);
}

void MainWindow::onConnectButtonClicked()
{
    ui->statusLabel->setText("Connecting...");
    m_client.connectToServer("127.0.0.1", 5555);
}

void MainWindow::onLoginButtonClicked()
{
    QString username = ui->usernameInput->text().trimmed();
    QString password = ui->passwordInput->text();

    if (username.isEmpty()) {
        ui->statusLabel->setText("Username is empty");
        return;
    }

    if (password.isEmpty()) {
        ui->statusLabel->setText("Password is empty");
        return;
    }

    ui->statusLabel->setText("Sending login...");
    m_client.sendLogin(username, password);
}

void MainWindow::onAddTaskButtonClicked()
{
    if (m_currentUsername.isEmpty()) {
        ui->statusLabel->setText("No logged user");
        return;
    }

    QString taskText = ui->taskInput->text().trimmed();

    if (taskText.isEmpty()) {
        ui->statusLabel->setText("Task text is empty");
        return;
    }

    ui->statusLabel->setText("Adding task...");
    m_client.sendAddTask(m_currentUsername, taskText);
}

void MainWindow::onClientConnected()
{
    ui->statusLabel->setText("Connected to server");
}

void MainWindow::onClientDisconnected()
{
    ui->statusLabel->setText("Disconnected from server");
}

void MainWindow::onLoginSuccess(const QString &username)
{
    setLoggedInUser(username);
    ui->statusLabel->setText("Login successful");

    m_client.sendGetTasks(username);
}

void MainWindow::onTaskAdded()
{
    ui->statusLabel->setText("Task added");
    ui->taskInput->clear();

    if (!m_currentUsername.isEmpty()) {
        m_client.sendGetTasks(m_currentUsername);
    }
}

void MainWindow::onTasksReceived(const QStringList &tasks)
{
    ui->taskList->clear();
    ui->taskList->addItems(tasks);
    ui->statusLabel->setText("Tasks loaded");
}

void MainWindow::onClientErrorOccurred(const QString &message)
{
    ui->statusLabel->setText("Error: " + message);
}

void MainWindow::setLoggedInUser(const QString &username)
{
    m_currentUsername = username;
    ui->currentUserLabel->setText("Current user: " + username);
}