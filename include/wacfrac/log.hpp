#pragma once

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <atomic>
#include <string>
#include <sstream>

namespace wacfrac::logging {


enum class Severity {
    Trace   = 0,
    Debug   = 1,
    Info    = 2,
    Warning = 3,
    Error   = 4,
    Fatal   = 5
};

inline std::atomic<bool>& do_log() {
    static std::atomic<bool> ENABLED {false};
    return ENABLED;
}
void init();

namespace detail {

inline void format_impl(std::ostringstream& oss, std::string_view fmt) {
    oss << fmt;
}

template<typename T, typename... Args>
void format_impl(std::ostringstream& oss, std::string_view fmt, T&& arg, Args&&... args) {
    auto pos = fmt.find("{}");
    if (pos == std::string_view::npos) {
        oss << fmt;
    } else {
        oss << fmt.substr(0, pos);
        oss << std::forward<T>(arg);
        format_impl(oss, fmt.substr(pos + 2), std::forward<Args>(args)...);
    }
}

} // namespace detail

template<typename... Args>
inline void print(Severity level, std::string_view fmt, Args&&... args) {
    if (!do_log())
        return;

    static boost::log::sources::severity_logger<Severity> logger {};
    static const char* strings[] = {
        "trace", "debug", "info", "warning", "error", "fatal"
    };
    auto label = static_cast<std::size_t>(level) < 6
        ? strings[static_cast<std::size_t>(level)]
        : std::to_string(static_cast<int>(level));

    std::ostringstream oss;
    oss << "[" << label << "] ";
    detail::format_impl(oss, fmt, std::forward<Args>(args)...);
    BOOST_LOG_SEV(logger, level) << oss.str();
}

} // namespace wacfrac::logging
