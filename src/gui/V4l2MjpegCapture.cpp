#include "V4l2MjpegCapture.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

namespace {
int query(int fd, unsigned long request, void *arg)
{
    int result;
    do { result = ioctl(fd, request, arg); } while (result < 0 && errno == EINTR);
    return result;
}
}
QString V4l2MjpegCapture::Mode::id() const
{
    return QString("%1x%2@%3/%4").arg(size.width()).arg(size.height())
        .arg(denominator).arg(numerator);
}
QString V4l2MjpegCapture::Mode::label() const
{
    return QString("%1 × %2 @ %3 fps (MJPEG)").arg(size.width()).arg(size.height())
        .arg(fps(), 0, 'f', std::abs(fps() - std::round(fps())) < 0.005 ? 0 : 2);
}
QList<V4l2MjpegCapture::Mode> V4l2MjpegCapture::modes(const QString &path)
{
    QList<Mode> result;
    int fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return result;
    v4l2_frmsizeenum size{};
    size.pixel_format = V4L2_PIX_FMT_MJPEG;
    for (; query(fd, VIDIOC_ENUM_FRAMESIZES, &size) == 0; ++size.index) {
        if (size.type != V4L2_FRMSIZE_TYPE_DISCRETE) continue;
        v4l2_frmivalenum interval{};
        interval.pixel_format = size.pixel_format;
        interval.width = size.discrete.width;
        interval.height = size.discrete.height;
        for (; query(fd, VIDIOC_ENUM_FRAMEINTERVALS, &interval) == 0; ++interval.index) {
            if (interval.type == V4L2_FRMIVAL_TYPE_DISCRETE &&
                interval.discrete.numerator && interval.discrete.denominator) {
                result.append({QSize(int(interval.width), int(interval.height)),
                    interval.discrete.numerator, interval.discrete.denominator});
            }
        }
    }
    ::close(fd);
    std::sort(result.begin(), result.end(), [](const Mode &a, const Mode &b) {
        if (a.size != b.size) return a.size.width() * a.size.height() > b.size.width() * b.size.height();
        return a.fps() > b.fps();
    });
    return result;
}
V4l2MjpegCapture::V4l2MjpegCapture(QString path, Mode mode, QObject *parent)
    : QThread(parent), m_path(std::move(path)), m_mode(mode) {}
V4l2MjpegCapture::~V4l2MjpegCapture()
{
    requestInterruption();
    wait();
}
void V4l2MjpegCapture::run()
{
    int fd = ::open(m_path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        emit captureFailed(QString("Cannot open %1: %2").arg(m_path, QString::fromLocal8Bit(strerror(errno))));
        return;
    }
    struct Buffer { void *data; size_t length; };
    std::vector<Buffer> buffers;
    bool streaming = false;
    QString error;
    auto fail = [&](const char *operation) {
        error = QString("%1: %2").arg(QString::fromLatin1(operation), QString::fromLocal8Bit(strerror(errno)));
    };
    // One cleanup path releases streaming buffers even when negotiation fails.
    do {
        v4l2_format format{};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = m_mode.size.width();
        format.fmt.pix.height = m_mode.size.height();
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        format.fmt.pix.field = V4L2_FIELD_ANY;
        if (query(fd, VIDIOC_S_FMT, &format) < 0) { fail("Set capture format"); break; }
        if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG ||
            int(format.fmt.pix.width) != m_mode.size.width() || int(format.fmt.pix.height) != m_mode.size.height()) {
            error = "Camera rejected the requested MJPEG resolution"; break;
        }
        v4l2_streamparm parm{};
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe = {m_mode.numerator, m_mode.denominator};
        if (query(fd, VIDIOC_S_PARM, &parm) < 0 || query(fd, VIDIOC_G_PARM, &parm) < 0) {
            fail("Set capture frame rate"); break;
        }
        const auto actual = parm.parm.capture.timeperframe;
        if (!actual.numerator || std::abs(double(actual.denominator) / actual.numerator - m_mode.fps()) > 0.01) {
            error = "Camera rejected the requested frame rate"; break;
        }
        v4l2_requestbuffers request{};
        request.count = 4;
        request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        request.memory = V4L2_MEMORY_MMAP;
        if (query(fd, VIDIOC_REQBUFS, &request) < 0 || !request.count) { fail("Allocate capture buffers"); break; }
        for (unsigned i = 0; i < request.count; ++i) {
            v4l2_buffer buffer{};
            buffer.type = request.type;
            buffer.memory = request.memory;
            buffer.index = i;
            if (query(fd, VIDIOC_QUERYBUF, &buffer) < 0) { fail("Query capture buffer"); break; }
            void *data = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
            if (data == MAP_FAILED) { fail("Map capture buffer"); break; }
            buffers.push_back({data, buffer.length});
            if (query(fd, VIDIOC_QBUF, &buffer) < 0) { fail("Queue capture buffer"); break; }
        }
        if (!error.isEmpty()) break;
        auto type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (query(fd, VIDIOC_STREAMON, &type) < 0) { fail("Start capture"); break; }
        streaming = true;
        int idlePolls = 0;
        while (!isInterruptionRequested()) {
            pollfd event{fd, POLLIN, 0};
            int ready = poll(&event, 1, 250);
            if (ready < 0 && errno == EINTR) continue;
            if (ready < 0) { fail("Poll capture"); break; }
            if (ready == 0) {
                if (++idlePolls >= 20) { error = "Camera did not deliver frames for five seconds"; break; }
                continue;
            }
            idlePolls = 0;
            if (event.revents & (POLLERR | POLLHUP | POLLNVAL)) { error = "Camera disconnected or capture failed"; break; }
            v4l2_buffer buffer{};
            buffer.type = type;
            buffer.memory = V4L2_MEMORY_MMAP;
            if (query(fd, VIDIOC_DQBUF, &buffer) < 0) {
                if (errno == EAGAIN) continue;
                fail("Read capture buffer"); break;
            }
            if (buffer.index >= buffers.size() || buffer.bytesused > buffers[buffer.index].length) {
                error = "Camera returned an invalid capture buffer"; break;
            }
            QImage image;
            if (!m_pending && !(buffer.flags & V4L2_BUF_FLAG_ERROR))
                image = QImage::fromData(static_cast<const uchar *>(buffers[buffer.index].data), int(buffer.bytesused), "JPEG");
            if (query(fd, VIDIOC_QBUF, &buffer) < 0) { fail("Return capture buffer"); break; }
            if (!image.isNull()) {
                m_pending = true; // At most one queued frame; do not grow a preview backlog.
                emit frameReady(image);
            }
        }
    } while (false);
    if (streaming) {
        auto type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        query(fd, VIDIOC_STREAMOFF, &type);
    }
    for (const auto &buffer : buffers) munmap(buffer.data, buffer.length);
    ::close(fd);
    if (!error.isEmpty() && !isInterruptionRequested()) emit captureFailed(error);
}
