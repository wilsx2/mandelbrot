#pragma once

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <string>
#include <sstream>

namespace wacfrac::logging {

enum class severity {
    trace   = 0,
    debug   = 1,
    info    = 2,
    warning = 3,
    error   = 4,
    fatal   = 5
};

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
inline void print(severity level, std::string_view fmt, Args&&... args) {
    static boost::log::sources::severity_logger<severity> logger {};
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
