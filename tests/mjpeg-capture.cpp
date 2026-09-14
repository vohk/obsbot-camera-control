// Opt-in hardware test. Decodes frames in memory; never records images or applies image controls.
#include "V4l2MjpegCapture.h"
#include "V4l2Backend.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <iostream>
#include <cmath>
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 3) return 2;
    const QString path = QString::fromLocal8Bit(argv[1]);
    const double fps = QString::fromLocal8Bit(argv[2]).toDouble();
    const auto modes = V4l2MjpegCapture::modes(path);
    for (const auto &mode : modes) {
        if (mode.size != QSize(1920, 1080) || std::abs(mode.fps() - fps) > 0.01) continue;
        V4l2MjpegCapture capture(path, mode);
        int frames = 0;
        QElapsedTimer timer;
        QObject::connect(&capture, &V4l2MjpegCapture::frameReady, &app, [&](const QImage &image) {
            capture.acknowledgeFrame();
            if (image.size() != mode.size) { app.exit(3); return; }
            if (!frames++) timer.start();
            if (timer.elapsed() >= 3000) {
                const double actual = V4l2Backend::frameRateForDevice(path.toStdString());
                std::cout << "negotiated=" << actual << " decoded=" << (frames - 1) * 1000.0 / timer.elapsed()
                          << " frames=" << frames << std::endl;
                app.exit(std::abs(actual - fps) < 0.01 && frames > fps * 2 ? 0 : 4);
            }
        });
        QObject::connect(&capture, &V4l2MjpegCapture::captureFailed, &app, [&](const QString &error) {
            std::cerr << error.toStdString() << std::endl; app.exit(1);
        });
        QTimer::singleShot(10000, &app, [&]() { app.exit(5); });
        capture.start();
        return app.exec();
    }
    std::cerr << "Requested 1080p interval not enumerated" << std::endl;
    return 6;
}
