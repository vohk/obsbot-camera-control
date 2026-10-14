#include "CaptureProgress.h"
#include <cstdlib>
#include <iostream>

void check(bool value, const char *message)
{
    if (!value) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

int main()
{
    using namespace std::chrono;
    const auto started = CaptureProgress::Time{};
    CaptureProgress startup(started);
    check(!startup.stalled(started), "startup waits for an actual buffer");
    check(!startup.stalled(started + milliseconds(4999)), "startup grace has not elapsed");
    // POLLIN without a successful DQBUF never calls bufferReceived.
    check(startup.stalled(started + seconds(5)), "EAGAIN/readiness cannot hide a stall");

    CaptureProgress running(started);
    running.bufferReceived(started + seconds(4));
    check(!running.stalled(started + seconds(5)), "dequeue resets the progress deadline");
    check(!running.stalled(started + milliseconds(8999)), "active capture retains its grace period");
    check(running.stalled(started + seconds(9)), "active capture can stall again");
    std::cout << "Capture progress checks passed; no devices opened\n";
}
