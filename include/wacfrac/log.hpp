#pragma once

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace wacfrac {

enum class severity_level {
    trace   = 0,
    debug   = 1,
    info    = 2,
    warning = 3,
    error   = 4,
    fatal   = 5
};

using logger = boost::log::sources::severity_logger<severity_level>;

namespace logging {

void init();
auto get_logger() -> logger&;

} // namespace logging

} // namespace wacfrac

#define LOG_TRACE   BOOST_LOG_SEV(wacfrac::logging::get_logger(), wacfrac::severity_level::trace)
#define LOG_DEBUG   BOOST_LOG_SEV(wacfrac::logging::get_logger(), wacfrac::severity_level::debug)
#define LOG_INFO    BOOST_LOG_SEV(wacfrac::logging::get_logger(), wacfrac::severity_level::info)
#define LOG_WARN    BOOST_LOG_SEV(wacfrac::logging::get_logger(), wacfrac::severity_level::warning)
#define LOG_ERROR   BOOST_LOG_SEV(wacfrac::logging::get_logger(), wacfrac::severity_level::error)
#define LOG_FATAL   BOOST_LOG_SEV(wacfrac::logging::get_logger(), wacfrac::severity_level::fatal)
