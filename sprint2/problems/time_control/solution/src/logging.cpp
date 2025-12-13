#include "logging.h"

#include <boost/log/support/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <iostream>

using boost::posix_time::to_iso_extended_string;
namespace json = boost::json;

void InitJsonLogging() {
    namespace logging = boost::log;
    namespace sinks = boost::log::sinks;
    namespace expr = boost::log::expressions;

    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    auto backend = boost::make_shared<sinks::text_ostream_backend>();
    backend->add_stream(boost::shared_ptr<std::ostream>(&std::cout, [](std::ostream*){}));
    backend->auto_flush(true);

    using sink_t = sinks::synchronous_sink<sinks::text_ostream_backend>;
    auto sink = boost::make_shared<sink_t>(backend);

    sink->set_formatter([](const logging::record_view& rec, logging::formatting_ostream& strm) {
        json::object root;

        // timestamp
        if (auto ts = rec[timestamp_attr]) {
            root["timestamp"] = to_iso_extended_string(*ts);
        }

        // data - дополнительные данные
        if (auto data = rec[additional_data]) {
            root["data"] = *data;
        } else {
            root["data"] = json::object{};
        }

        // message
        if (auto msg = rec[expr::smessage]) {
            root["message"] = msg.get();
        }

        strm << json::serialize(root) << "\n";
    });

    logging::core::get()->add_sink(sink);
    logging::add_common_attributes();
}

