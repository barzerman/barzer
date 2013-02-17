
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
/*
 * ay_logger.h
 *
 *  Created on: Apr 11, 2011
 *      Author: polter
 */

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <ay_headers.h>

#include <arch/barzer_arch.h>

#ifndef LOG_DISABLE
#define AYLOG(l) ay::LogMsg(ay::Logger::l,__FILE__,__LINE__).getStream()
#else
#define AYLOG(l) StreamWrap()
#endif

#define AYLOGDEBUG(l) AYLOG(DEBUG) << #l << " = " << (l)
#define AYLOGINIT(l) ay::Logger::init(ay::Logger::l);
#define SETLOGLEVEL(l) ay::Logger::LEVEL = ay::Logger::l

namespace ay {

struct StreamWrap {
    std::ostream* os;
    StreamWrap(std::ostream* o=0) : os(o) {}
};
inline StreamWrap operator<<(StreamWrap vs, std::ostream& ( *pf )(std::ostream&)) { 
 if( vs.os ) pf(*(vs.os));
  return vs;
}

inline StreamWrap operator<<(StreamWrap vs, std::ios& ( *pf )(std::ios&)) { 
   if( vs.os ) pf(*(vs.os));
    return vs;
}

inline StreamWrap operator<<(StreamWrap vs, std::ios_base& ( *pf )(std::ios_base&)) { 
 if( vs.os ) pf(*(vs.os));
  return vs;
}
template <typename T>
inline StreamWrap operator<<(StreamWrap vs, const T& t) {
 if( vs.os ) *(vs.os) << t; 
  return vs;
}
inline StreamWrap operator<<(StreamWrap vs, const char t[]) {
 if( vs.os ) *(vs.os) << t; 
  return vs;
}

/*
class VoidStream {};

template<class T>
inline VoidStream& operator<<(VoidStream &vs, const T &v) { return vs; }
*/


// this is awful, should probably just replace it with a bunch of static functions
class Logger {
public:
	//static VoidStream voidstream;
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

	std::ostream* getStream() { 
        if( stream_ )
            return stream_; 
        else 
            return &std::cerr;
    }


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
         StreamWrap sw = getStream();
         if( sw.os )
		    (*(sw.os) << "\n").flush();

	}
	StreamWrap getStream();

private:

};

/// if fileName is not an absolute path deduces current directory and prints it
std::ostream& print_absolute_file_path( std::ostream& , const char* fileName );
} // namespace ay

