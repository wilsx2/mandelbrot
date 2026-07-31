#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <iostream>
#include <wacfrac/log.hpp>

namespace wacfrac::logging
{

void init(int level)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    log_level() = level;

    auto sink = boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend>>();

    sink->locked_backend()->add_stream(boost::make_shared<std::ostream>(std::cout.rdbuf()));

    sink->set_formatter(boost::log::expressions::stream
                        << "["
                        << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
                        << "] " << boost::log::expressions::smessage);

    boost::log::core::get()->add_sink(sink);
    boost::log::add_common_attributes();
}

} // namespace wacfrac::logging
