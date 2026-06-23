#include <wacfrac/log.hpp>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <iostream>

namespace wacfrac {

auto logging::get_logger() -> logger&  {
    static logger log;
    return log;
}

void logging::init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    auto sink = boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend>>();

    sink->locked_backend()->add_stream(
        boost::make_shared<std::ostream>(std::cout.rdbuf()));

    sink->set_formatter(
        boost::log::expressions::stream
            << "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
            << "][" << boost::log::expressions::attr<severity_level>("Severity")
            << "] " << boost::log::expressions::smessage
    );

    boost::log::core::get()->add_sink(sink);
    boost::log::add_common_attributes();
}

std::ostream& operator<<(std::ostream& strm, severity_level lvl) {
    static const char* strings[] = {
        "trace", "debug", "info", "warning", "error", "fatal"
    };
    if (static_cast<std::size_t>(lvl) < 6)
        strm << strings[static_cast<std::size_t>(lvl)];
    else
        strm << static_cast<int>(lvl);
    return strm;
}

} // namespace wacfrac
