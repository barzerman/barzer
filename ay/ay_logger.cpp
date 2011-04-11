/*
 * ay_logger.cpp
 *
 *  Created on: Apr 11, 2011
 *      Author: polter
 */

#include <iostream>
#include <fstream>
#include "ay_logger.h"


namespace ay {

Logger *Logger::instance_;
int Logger::LEVEL;
VoidStream Logger::voidstream;
//std::ostream *Logger::stream_;


std::ostream& LogMsg::getStream() {
	// return (level_ >= logger_->LEVEL) ? logger_->getStream() : voidstream;
	if (level_ >= logger_->LEVEL) return logger_->getStream();
	else return Logger::voidstream;
}

void Logger::init(int l = ERROR) {
	LEVEL = l;
}


Logger* Logger::getLogger() {
	if (!instance_) instance_ = new Logger();
	return instance_;
}


void Logger::setFile(const char* filename) {
	stream_ = new std::ofstream(filename);
}

void Logger::setStream(std::ostream* os) {
	stream_ = os;
}

std::ostream& Logger::logMsg(const int lvl, const char* filename, const int lineno) {
	if(lvl > 4) return *stream_; // should probably crash right here
	if (lvl >= LEVEL) {
		*stream_ << filename << ":" << lineno << ":[" << LOG_LVL_STR[lvl] << "] ";
	}
}

}
