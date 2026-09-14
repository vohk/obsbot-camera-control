// Opt-in hardware regression check: reads camera state, never applies settings.
#include "CameraController.h"
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    CameraController controller;
    const bool cancel = app.arguments().contains("--cancel");
    QObject::connect(&controller, &CameraController::cameraConnected, &app,
        [&](const CameraController::CameraInfo &info) {
            std::cout << "backend=" << (controller.isV4l2Only() ? "V4L2" : "SDK")
                      << " product=" << info.productType << std::endl;
            app.exit(cancel || controller.isV4l2Only() ? 1 : 0);
        });
    QTimer::singleShot(12000, &app, [&]() {
        if (cancel) std::cout << "cancelled discovery stayed disconnected" << std::endl;
        app.exit(cancel ? 0 : 2);
    });
    QTimer::singleShot(0, &controller, [&]() {
        controller.connectToCamera();
        if (cancel) controller.disconnectFromCamera();
    });
    return app.exec();
}
