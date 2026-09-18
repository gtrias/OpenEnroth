#include "StackTrace.h"

#include <cstdio>
#include <string>

#if !defined(__ANDROID__) && !defined(OE_BUILD_VITA)
#   include <cpptrace/cpptrace.hpp>
#endif

#include "Utility/String/Format.h"

#if defined(__ANDROID__) || defined(OE_BUILD_VITA)

std::string stackTraceToString() {
    return "Stack traces not supported on Android...";
}

#else

std::string stackTraceToString() {
    return cpptrace::generate_trace(0, detail::MAX_TRACE_DEPTH).to_string();
}

#endif

void printStackTrace(FILE *stream) {
    fmt::println(stream, "{}", stackTraceToString());
}
