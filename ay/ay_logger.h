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

#ifndef LOG_DISABLE
#define AYLOG(l) ay::LogMsg(ay::Logger::l,__FILE__,__LINE__).getStream()
#else
#define AYLOG(l) ay::Logger::voidstream
#endif

namespace ay {

// just for the purpose of overloading  operator<< for supressed log levels
// and again i'm not sure if i'm doing this correctly

class VoidStream : public std::ostream {};

template<class T>
inline VoidStream& operator<<(VoidStream &vs, const T &v) { return vs; }




// A message clas. Made it for the sake of using destructor to add
// a linebreak at the end. No I'm not sure this is the best way to handle it.

// this is awful, should probably just replace it by a bunch of static functions
const std::string LOG_LVL_STR[] = {"DEBUG", "WARNING","ERROR","CRITICAL", "KABOOM"};
class Logger {
public:
	static VoidStream voidstream;
	static int LEVEL;
	enum LogLevelEnum {
		DEBUG,
		WARNING,
		ERROR,
		CRITICAL,
		KABOOM // something awful which should stop program running immediately
	};


	static void init(int l);

	static Logger* getLogger();

	static void setLevel(int l) { LEVEL = l; }

	std::ostream& logMsg(const int lvl, const char* filename,
										   const int lineno);

	std::ostream& getStream() { return *stream_; }


	void setFile(const char* filename);
	void setStream(std::ostream* stream);

private:
	static Logger *instance_;
	std::ostream *stream_;
	bool gotfile;
	Logger() : stream_(&std::cerr), gotfile(false) {}
};


class LogMsg {
	int level_;
	Logger* logger_;
public:
	LogMsg(const int lvl, const char* filename, const int lineno) :
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

