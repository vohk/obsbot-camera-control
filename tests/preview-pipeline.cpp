// Opt-in GUI/hardware check: preview -> GPU filter -> snapshot in memory only.
#include "CameraPreviewWidget.h"
#include "VirtualCameraStreamer.h"
#include <QApplication>
#include <QComboBox>
#include <QTimer>
#include <iostream>
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc != 2) return 2;
    const QString path = QString::fromLocal8Bit(argv[1]);
    CameraPreviewWidget preview;
    preview.resize(640, 420);
    preview.setCameraDeviceId(path);
    preview.setHdrEnabled(true);
    preview.show();
    int stage = 0;
    QObject::connect(&preview, &CameraPreviewWidget::previewFailed, &app, [&](const QString &error) {
        std::cerr << error.toStdString() << std::endl; app.exit(1);
    });
    QObject::connect(&preview, &CameraPreviewWidget::previewStarted, &app, [&]() {
        auto *combo = preview.findChild<QComboBox *>();
        if (!combo) { app.exit(3); return; }
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemText(i).contains("no HDR")) { app.exit(4); return; }
        }
        preview.captureSnapshot();
    });
    QObject::connect(&preview, &CameraPreviewWidget::snapshotCaptured, &app, [&](const QImage &image) {
        if (image.size() != QSize(1920, 1080)) { app.exit(5); return; }
        std::cout << "GPU preview frame " << preview.preferredFormatId().toStdString() << std::endl;
        if (stage++ == 1) { app.quit(); return; }
        auto *combo = preview.findChild<QComboBox *>();
        for (const auto &mode : V4l2MjpegCapture::modes(path)) {
            if (mode.size == QSize(1920, 1080) && qAbs(mode.fps() - 30) < 0.01) {
                combo->setCurrentIndex(combo->findData(mode.id()));
                return;
            }
        }
        app.exit(6);
    }, Qt::QueuedConnection); // Never close/reconfigure a window from inside paintGL.

    QTimer::singleShot(12000, &app, [&]() { app.exit(7); });
    QTimer::singleShot(0, &preview, [&]() { preview.enablePreview(true); });
    const int result = app.exec();
    preview.enablePreview(false);
    return result;
}
