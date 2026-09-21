#include "ScriptLogSink.h"

#include <sol/sol.hpp>

#include "Library/Serialization/Serialization.h"

ScriptLogSink::ScriptLogSink(sol::state_view solState) : _solState(solState) {}

void ScriptLogSink::write(const LogCategory& category, LogLevel level, std::string_view message) {
    // A log sink must never throw: sol2 raises a C++ exception when the Lua call fails, and an exception escaping a
    // sink during error reporting turns "log the error and rethrow" into an unexplained std::terminate.
    try {
        if (sol::protected_function logSink = _solState["_globalLogSink"]) {
            logSink(toString(level), message);
        }
    } catch (...) {
        // Nothing sensible to do here - the message we were asked to log is already going to the other sinks.
    }
}
