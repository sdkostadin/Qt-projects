# 🌐 Client-Server Task Manager

## 📌 Overview

Client-Server Task Manager is a simple application built with **C++ + Qt
(Widgets + Network)** that demonstrates communication between a client
and a server.

The system allows users to log in and retrieve their associated tasks
from the server.

------------------------------------------------------------------------

## 🧩 Features

### Client

-   GUI built with Qt Widgets\
-   User login (username + password)\
-   Sends requests to the server\
-   Displays user-specific tasks

### Server

-   Handles multiple client connections\
-   Validates user credentials\
-   Returns tasks for authenticated users

------------------------------------------------------------------------

## ⚙️ Core Concept

1.  Client connects to the server\
2.  User credentials are sent\
3.  Server validates against stored data\
4.  If valid → tasks are returned\
5.  Client displays the result

------------------------------------------------------------------------

## 📂 Data Storage

User data and tasks are stored in JSON files:

-   users.json → stores usernames and passwords\
-   tasks.json → stores tasks per user

------------------------------------------------------------------------

## 🛠️ Technologies

-   C++\
-   Qt (Widgets, Network)\
-   JSON\
-   CMake


