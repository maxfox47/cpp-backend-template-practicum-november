#pragma once

#include <boost/json.hpp>
#include <boost/log/core.hpp>			 // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp> // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace logging = boost::log;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(ip_address, "IP", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(uri, "URI", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(http_method, "Method", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(response_time, "ResponseTime", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(status_code, "StatusCode", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(content_type, "ContentType", std::string)

BOOST_LOG_ATTRIBUTE_KEYWORD(ip_add, "address", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(port_p, "port", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(exception_c, "exception", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(status_c, "code", std::string)

BOOST_LOG_ATTRIBUTE_KEYWORD(error_code, "ec", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(text, "text", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(where, "where", std::string)

inline void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	auto ts = rec[timestamp];
	auto message = rec[boost::log::expressions::smessage];

	json::object log_entry;

	log_entry["timestamp"] = boost::posix_time::to_iso_extended_string(*ts);

	json::object data_obj;

	auto ip_attr = rec[ip_address];
	if (ip_attr) {
		data_obj["ip"] = ip_attr->c_str();
	}

	auto uri_attr = rec[uri];
	if (uri_attr) {
		data_obj["URI"] = uri_attr->c_str();
	}

	auto method_attr = rec[http_method];
	if (method_attr) {
		data_obj["method"] = method_attr->c_str();
	}

	// response
	auto response_time_attr = rec[response_time];
	if (response_time_attr) {
		data_obj["response_time"] = *response_time_attr;
	}

	auto status_code_attr = rec[status_code];
	if (status_code_attr) {
		data_obj["code"] = *status_code_attr;
	}

	auto content_type_attr = rec[content_type];
	if (content_type_attr) {
		data_obj["content_type"] = content_type_attr->c_str();
	}
	// start
	auto port_attr = rec[port_p];
	if (port_attr) {
		data_obj["port"] = *port_attr;
	}
	auto address_attr = rec[ip_add];
	if (address_attr) {
		data_obj["address"] = address_attr->c_str();
	}
	// end
	auto status_attr = rec[status_c];
	if (status_attr) {
		data_obj["code"] = status_attr->c_str();
	}

	auto exception_attr = rec[exception_c];
	if (exception_attr) {
		data_obj["exception"] = exception_attr->c_str();
	}
	// error
	auto text_attr = rec[text];
	if (text_attr) {
		data_obj["text"] = text_attr->c_str();
	}

	auto where_attr = rec[where];
	if (where_attr) {
		data_obj["where"] = where_attr->c_str();
	}

	auto error_code_attr = rec[error_code];
	if (error_code_attr) {
		data_obj["code"] = *error_code_attr;
	}

	log_entry["data"] = data_obj;
	log_entry["message"] = *message;

	strm << log_entry;
}

