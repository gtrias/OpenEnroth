#include "VitaDiagnostics.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

#include "Library/Logger/Logger.h"

#include "VitaMemory.h"

namespace {
std::atomic<uint64_t> mainLoopTicks = 0;
std::atomic<int64_t> lastTickMs = 0;

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void diagnosticsThread(int stallSeconds) {
    constexpr int64_t HEARTBEAT_MS = 10000;
    uint64_t previousTicks = mainLoopTicks.load();
    int64_t previousHeartbeatMs = nowMs();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int64_t now = nowMs();
        int64_t lastTick = lastTickMs.load();
        int64_t stallMs = lastTick ? now - lastTick : 0;

        if (stallSeconds > 0 && stallMs > stallSeconds * 1000ll) {
            MM_CRITICAL("Main loop stalled for {} s, crashing on purpose so the core dump shows the main thread.",
                        stallMs / 1000);
            __builtin_trap();
        }

        if (now - previousHeartbeatMs >= HEARTBEAT_MS) {
            uint64_t ticks = mainLoopTicks.load();
            float fps = (ticks - previousTicks) * 1000.0f / (now - previousHeartbeatMs);
            MM_INFO("Heartbeat: {:.1f} fps, main loop last ticked {} ms ago.", fps, stallMs);
            logVitaMemoryUsage("heartbeat");
            previousTicks = ticks;
            previousHeartbeatMs = now;
        }
    }
}
} // namespace

void startVitaDiagnostics(int stallSeconds) {
    MM_INFO("Vita diagnostics started, main loop stall limit {} s (0 = off).", stallSeconds);
    std::thread(diagnosticsThread, stallSeconds).detach();
}

void vitaMainLoopTick() {
    mainLoopTicks.fetch_add(1, std::memory_order_relaxed);
    lastTickMs.store(nowMs(), std::memory_order_relaxed);
}
