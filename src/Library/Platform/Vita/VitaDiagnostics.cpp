#include "VitaDiagnostics.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/cpu.h>
#include <psp2/kernel/threadmgr.h>

#include "Library/Logger/Logger.h"

namespace {
std::atomic<uint64_t> mainLoopTicks = 0;
std::atomic<int64_t> lastTickMs = 0;
std::atomic<int64_t> longestTickMs = 0;
std::atomic<int> currentScreen = -1;

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void watchdogThread(int stallSeconds) {
    // A main thread spinning at the default priority on the same core would starve this thread, so run above it on
    // a core of its own. It never logs: a stall can come from file I/O hanging, and a log write would hang it too.
    sceKernelChangeThreadPriority(sceKernelGetThreadId(), 64);
    sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_2);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (nowMs() - lastTickMs.load() > stallSeconds * 1000ll)
            __builtin_trap();
    }
}

void heartbeatThread() {
    constexpr int64_t HEARTBEAT_MS = 10000;
    constexpr const char *DUMP_REQUEST_PATH = "ux0:/data/OpenEnroth/vita-dump-now";
    uint64_t previousTicks = mainLoopTicks.load();
    int64_t previousHeartbeatMs = nowMs();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // `vita.sh dump` creates this file to get a core dump of a running game, e.g. to see where a slow frame goes.
        SceIoStat stat;
        if (sceIoGetstat(DUMP_REQUEST_PATH, &stat) >= 0) {
            sceIoRemove(DUMP_REQUEST_PATH);
            MM_INFO("Core dump requested, crashing on purpose.");
            __builtin_trap();
        }

        int64_t now = nowMs();
        if (now - previousHeartbeatMs < HEARTBEAT_MS)
            continue;
        uint64_t ticks = mainLoopTicks.load();
        float fps = (ticks - previousTicks) * 1000.0f / (now - previousHeartbeatMs);
        MM_INFO("Heartbeat: {:.1f} fps, longest frame {} ms, screen {}, main loop last ticked {} ms ago.", fps,
                longestTickMs.exchange(0), currentScreen.load(), now - lastTickMs.load());
        previousTicks = ticks;
        previousHeartbeatMs = now;
    }
}
} // namespace

void startVitaDiagnostics(int stallSeconds) {
    MM_INFO("Vita diagnostics started, main loop stall limit {} s (0 = off).", stallSeconds);
    lastTickMs.store(nowMs());
    std::thread(heartbeatThread).detach();
    if (stallSeconds > 0)
        std::thread(watchdogThread, stallSeconds).detach();
}

void vitaMainLoopTick(int screen) {
    int64_t now = nowMs();
    int64_t interval = now - lastTickMs.exchange(now, std::memory_order_relaxed);
    if (interval > longestTickMs.load(std::memory_order_relaxed))
        longestTickMs.store(interval, std::memory_order_relaxed);
    currentScreen.store(screen, std::memory_order_relaxed);
    mainLoopTicks.fetch_add(1, std::memory_order_relaxed);
}
