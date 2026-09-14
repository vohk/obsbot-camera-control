#pragma once

#include <QImage>
#include <QSize>
#include <QThread>
#include <atomic>

// Qt exposes a min/max interval range and always selects its maximum. Use
// V4L2 directly for MJPEG so each advertised discrete interval is selectable.
class V4l2MjpegCapture : public QThread
{
    Q_OBJECT
public:
    struct Mode {
        QSize size;
        unsigned numerator = 0;
        unsigned denominator = 0;
        double fps() const { return numerator ? double(denominator) / numerator : 0; }
        QString id() const;
        QString label() const;
    };
    static QList<Mode> modes(const QString &path);
    V4l2MjpegCapture(QString path, Mode mode, QObject *parent = nullptr);
    ~V4l2MjpegCapture() override;
    void acknowledgeFrame() { m_pending = false; }
signals:
    void frameReady(const QImage &image);
    void captureFailed(const QString &message);
protected:
    void run() override;
private:
    QString m_path;
    Mode m_mode;
    std::atomic_bool m_pending{false};
};
