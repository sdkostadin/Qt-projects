#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "codingcalculator.h"
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Coding Calculator");

    CodingCalculator calculator;
    calculator.loadDevices();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("codingCalculator", &calculator);
    engine.loadFromModule("CodingCalculator", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
