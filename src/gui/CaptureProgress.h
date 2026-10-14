#ifndef CAPTUREPROGRESS_H
#define CAPTUREPROGRESS_H
#include <chrono>

// Buffer progress is separate from image validity and from poll readiness.
class CaptureProgress
{
public:
    using Clock = std::chrono::steady_clock;
    using Time = Clock::time_point;

    explicit CaptureProgress(Time started) : m_lastBuffer(started) {}
    void bufferReceived(Time now) { m_lastBuffer = now; }
    bool stalled(Time now) const
    {
        return now - m_lastBuffer >= std::chrono::seconds(5);
    }

private:
    Time m_lastBuffer;
};
#endif
