#ifndef __LIB_LOGGER_H__
#define __LIB_LOGGER_H__


#include <stdio.h>
#include <string>
#include <fstream>
#include <boost/format.hpp>

using namespace std;

#include "typeinfo/t_errors.h"
#include "config.h"

/*
 * Log System
 *
 * TODO
 *
 *  > template based log w/ conversion to string (eg. Log::Log(1234); )
 *  > allow log levels
 *  > std output w/ colours?
 *  > macro LOG("%1%: %2% is %3%", title, name, reason) <-- Log(str(format(...) % .. % .. % ..))
 *
 ***/


/*
=================================================

	Log

	simple logging system

=================================================
*/
namespace Logger {

	const int LOG_DEBUG   = 0;
	const int LOG_INFO    = 1;
	const int LOG_WARNING = 2;
	const int LOG_ERROR   = 3;


	t_error Log(const string msg, const int level = Logger::LOG_DEBUG);

	namespace LogSystem {
		static int logLevel;
		static ofstream *logFile;

		static const char* LOG_DEBUG_HDR   = "%s";
		static const char* LOG_WARNING_HDR = "WARNING: %s";
		static const char* LOG_ERROR_HDR   = "ERROR: %s";

		t_error startup(const char* filename = CFG_LOGFILE);
		t_error shutdown();

		t_error printLog(const string& msg); // print log to std output
		t_error writeLog(const string& msg); // write log to file

		string  getLogColor(const int level); // retrieve colour represnting this log level
	}

};


#endif
