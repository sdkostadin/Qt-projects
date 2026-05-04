#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <BitGrid.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    BitGrid grid;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("BitGrid",&grid);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("BitGrid", "Main");

    return QCoreApplication::exec();
}
