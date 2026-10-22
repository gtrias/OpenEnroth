#pragma once

/**
 * Starts a background thread that logs a heartbeat line every 10 seconds with the main loop's frame rate and the
 * memory numbers. With a stall limit set, the thread also crashes the process on purpose once the main loop has
 * stopped ticking for that long, so the core dump shows where the main thread is stuck. The limit only applies after
 * the first tick, startup loading doesn't tick.
 *
 * @param stallSeconds                  Main loop stall that triggers the core dump, 0 disables it.
 */
void startVitaDiagnostics(int stallSeconds);

/**
 * Marks one main loop iteration for `startVitaDiagnostics`.
 */
void vitaMainLoopTick();
