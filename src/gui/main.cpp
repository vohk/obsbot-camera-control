#include <QApplication>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QTextStream>
#include "MainWindow.h"
#include "VirtualCameraStreamer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Packaging smoke test: initialize Qt/plugins without applying camera settings.
    if (app.arguments().contains(QStringLiteral("--check-runtime"))) {
        QTextStream(stdout) << "Qt " << qVersion() << ", platform "
                            << app.platformName() << ", cameras "
                            << QMediaDevices::videoInputs().size() << Qt::endl;
        QTextStream(stdout) << "Loopback devices: " << VirtualCameraStreamer::availableDevices().join(", ") << Qt::endl;
        return 0;
    }

    MainWindow window;
    if (!window.isStartingMinimized()) {
        window.show();
    }

    return app.exec();
}
