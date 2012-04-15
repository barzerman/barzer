/*
 * ay_logger.cpp
 *
 *  Created on: Apr 11, 2011
 *      Author: polter
 */

#include <ay_logger.h>


namespace ay {

Logger *Logger::instance_ = 0;
uint8_t Logger::LEVEL = Logger::WARNING;
VoidStream Logger::voidstream;
namespace {
const char* g_LOG_LVL_STR[] = {"DEBUG", "WARNING","ERROR","CRITICAL"};
}
const char* ay::Logger::getLogLvlStr( int x)
{
	return( x>=0 && (unsigned)(x)< sizeof(g_LOG_LVL_STR)/sizeof(g_LOG_LVL_STR[0]) ? g_LOG_LVL_STR[x]: "");

}


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
		return (*stream_ << filename << ":" << lineno << ":[" << g_LOG_LVL_STR[lvl]
				<< "] ") ;
	}
	return *stream_;
}

#if (defined (_WINDOWS_) || defined(WIN32) || defined(_MSC_VER))
inline char *getcwd(char *buf, size_t size)
{
    /// maybe some day someone will implement getcwd for windows (AY 04/15/2012))
    return ( (size? (*buf=0):0 ), "" );
}
#else
#include <unistd.h>
#endif
std::ostream& print_absolute_file_path( std::ostream& fp, const char* fileName )
{
    if( !fileName ) 
        return fp;
    if( *fileName != '/' ) {
        // if we're here fileName is not an absolute path
        char curDir[1024]; 
        curDir[0] = curDir[ sizeof(curDir)-1 ] = 0;
        if( getcwd(curDir, sizeof(curDir)-1) ) 
            fp << curDir << "/";
    }
    return ( fp << fileName );
}

} // namespace ay
