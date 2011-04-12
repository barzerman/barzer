/*
 * ay_logger.cpp
 *
 *  Created on: Apr 11, 2011
 *      Author: polter
 */

#include "ay_logger.h"

namespace ay {

Logger *Logger::instance_ = 0;
uint8_t Logger::LEVEL = Logger::WARNING;
VoidStream Logger::voidstream;
const std::string Logger::LOG_LVL_STR[Logger::LOG_LEVEL_MAX]
                                      = {"DEBUG", "WARNING","ERROR","CRITICAL"};

std::ostream& LogMsg::getStream()
{
	// return (level_ >= logger_->LEVEL) ? logger_->getStream() : voidstream;
	if ( level_ >= logger_->LEVEL )
		return logger_->getStream();
	else
		return Logger::voidstream;
}

void Logger::init( uint8_t l = WARNING ) { LEVEL = l; }

Logger* Logger::getLogger()
{
	if ( !instance_ )
		instance_ = new Logger();
	return instance_;
}

void Logger::setFile( const char* filename )
{
	if ( gotfile )
		delete stream_;
	gotfile = true;
	stream_ = new std::ofstream( filename );
}

void Logger::setStream( std::ostream* os )
{
	if ( gotfile ) {
		delete stream_;
		gotfile = false;
	}
	stream_ = os;
}

std::ostream& Logger::logMsg( const uint8_t lvl, const char* filename,
		const int lineno )
{
	if (lvl >= Logger::LOG_LEVEL_MAX)
		return *stream_; // should probably crash right here
	if ( lvl >= LEVEL ) {
		return (*stream_ << filename << ":" << lineno << ":[" << LOG_LVL_STR[lvl]
				<< "] ") ;
	}
}

}
