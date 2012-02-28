/*
 * ay_logger.h
 *
 *  Created on: Apr 11, 2011
 *      Author: polter
 */

#ifndef AY_LOGGER_H_
#define AY_LOGGER_H_

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <ay_headers.h>
#include <arch/barzer_arch.h>

#ifndef LOG_DISABLE
#define AYLOG(l) ay::LogMsg(ay::Logger::l,__FILE__,__LINE__).getStream()
#else
#define AYLOG(l) ay::Logger::voidstream
#endif

#define AYLOGDEBUG(l) AYLOG(DEBUG) << #l << " = " << (l)
#define AYLOGINIT(l) ay::Logger::init(ay::Logger::l);
#define SETLOGLEVEL(l) ay::Logger::LEVEL = ay::Logger::l

namespace ay {

// just for the purpose of overloading  operator<< for supressed log levels
// and again i'm not sure if i'm doing this correctly

class VoidStream : public std::ostream {};

template<class T>
inline VoidStream& operator<<(VoidStream &vs, const T &v) { return vs; }


// this is awful, should probably just replace it with a bunch of static functions
class Logger {
public:
	static VoidStream voidstream;
	static uint8_t LEVEL;

	enum LogLevelEnum {
		DEBUG,
		WARNING,
		ERROR,
		CRITICAL,

		LOG_LEVEL_MAX// something awful which should stop program running immediately
	};
	const static std::string LOG_LVL_STR[LOG_LEVEL_MAX];

	static void init(uint8_t l);

	static Logger* getLogger();

	static void setLevel(uint8_t l) { LEVEL = l; }

	std::ostream& logMsg(const uint8_t lvl, const char* filename,
										   const int lineno);

	std::ostream& getStream() { return *stream_; }


	void setFile(const char* filename);
	void setStream(std::ostream* stream);
	static const char* getLogLvlStr( int );
private:
	static Logger *instance_;
	std::ostream *stream_;
	bool gotfile;
	Logger() : stream_(&std::cerr), gotfile(false) {}
};


// A message class. Made it for the sake of using the destructor to add
// a linebreak at the end. No I'm not sure this is the best way to handle it.

class LogMsg {
	uint level_;
	Logger* logger_;
public:
	LogMsg(const uint8_t lvl, const char* filename, const int lineno) :
		level_(lvl), logger_(Logger::getLogger())
	{
		logger_->logMsg(lvl, filename, lineno);
	}
	~LogMsg()
	{
		 (getStream() << "\n").flush();

	}
	std::ostream& getStream();

private:

};

}


#endif /* AY_LOGGER_H_ */

