#include <Logger.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/date_time/date_defs.hpp>

void initializeLog(int logLevel, int rotationSizeMB, int maxFileCount, std::string logFilename)
{
	//Convert log level to boost's severity_level

	boost::log::trivial::severity_level boostLogLevel;

	switch(logLevel)
	{
	case 1:
		boostLogLevel = boost::log::trivial::error;
		break;
	case 2:
		boostLogLevel = boost::log::trivial::warning;
		break;
	case 3:
		boostLogLevel = boost::log::trivial::info;
		break;
	case 4:
		boostLogLevel = boost::log::trivial::debug;
		break;
	case 5:
		boostLogLevel = boost::log::trivial::trace;
		break;
	default:
		boostLogLevel = boost::log::trivial::info;
	}
	boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");	//to register "Severity" as a format string

	logFilename += "_%N.log";
	
	auto sink = boost::log::add_file_log
		(
		boost::log::keywords::file_name = logFilename.c_str(),
		boost::log::keywords::rotation_size = rotationSizeMB * 1024 * 1024,
		//boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(boost::date_time::Sunday, 0, 0, 0),
		boost::log::keywords::format = "[%TimeStamp%] [%Severity%]: %Message%",
		boost::log::keywords::auto_flush = true
		);

	sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
		boost::log::keywords::target = ".",
		boost::log::keywords::max_size = rotationSizeMB * 1024 * 1024 * maxFileCount
		));
		
	boost::log::core::get()->set_filter
		(
		boost::log::trivial::severity >= boostLogLevel
		);

	boost::log::add_common_attributes();
}

