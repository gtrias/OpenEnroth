#pragma once

#include <string_view>

/**
 * Logs free memory, both the console's own and vitaGL's pools. This port has a documented memory budget problem -
 * the mmap shim never frees - so knowing the numbers at each stage is what tells a crash apart from an OOM.
 */
void logVitaMemoryUsage(std::string_view stage);
