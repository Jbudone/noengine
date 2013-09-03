#include "lib_logger.h"



t_error Logger::Log(const string msg, const int level /* = Logger::LOG_DEBUG */) {
	if ( level < Logger::LogSystem::logLevel ) return NO_ERROR;
	t_error err = NO_ERROR;
	err |= Logger::LogSystem::printLog( Logger::LogSystem::getLogColor(level) + msg + "\n" + KNRM );
	err |= Logger::LogSystem::writeLog( msg + "\n" );
	return err;
}

t_error Logger::LogSystem::startup(const char* filename) {
	Logger::LogSystem::logLevel = CFG_LOGLEVEL;
	Logger::LogSystem::logFile = new ofstream( filename );
	if ( Logger::LogSystem::logFile->is_open() == false ) {
		return ERROR;
	}
	
	return 0;
}

t_error Logger::LogSystem::shutdown() {
	Logger::LogSystem::logFile->close();
	printf( KNRM );
	return 0;
}

t_error Logger::LogSystem::printLog(const string& msg) {
	cout << msg.c_str();
	// printf( msg.c_str() );
	return 0;
}

t_error Logger::LogSystem::writeLog(const string& msg) {
	(*Logger::LogSystem::logFile) << msg.c_str();
	return 0;
}

string Logger::LogSystem::getLogColor(const int level) {
	switch ( level ) {
		case LOG_DEBUG:
			return KGRN; break;
		case LOG_INFO:
			return KCYN; break;
		case LOG_WARNING:
			return KYEL; break;
		case LOG_ERROR:
			return KRED; break;
		default:
			return KNRM; break;
	}
}

