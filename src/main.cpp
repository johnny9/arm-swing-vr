// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_controller.h"
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("Arm Swing VR");
    QCoreApplication::setApplicationVersion(ARMSWING_VERSION);
    QCoreApplication::setOrganizationName("Arm Swing VR");
    QQuickStyle::setStyle("Basic");
    QCommandLineParser parser;
    parser.setApplicationDescription("Linux VR arm-swing locomotion — profile editor preview");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);
    ProfileController controller(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ui.ini");
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}});
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
