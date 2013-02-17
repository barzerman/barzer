
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay_headers.h>

#include <arch/barzer_arch.h>

#include <iostream>
#include <iomanip>
namespace ay {

/// stopwatch is a cumulative timer - wrapper of gettimeofday
/// starts on construction
/// standard usage:
/// ay::stopwatch sw;
///  std::cout << sw.calcTime() << "\n";
///  or if you need to pause the watch 
/// sw.stop() ... do something else 
/// sw.start() 
struct stopwatch 
{
	time_t sec; // total elapsed seconds`
	suseconds_t msec; // total elapsed microseconds

	struct timeval tmpTv;

	stopwatch();
	/// if advance == true advances the times 
	/// advance true is equivalent to stop()
	stopwatch& calcTime(bool advance= false);
	stopwatch& stop()
		{ calcTime(true); return *this; }
	stopwatch& start();
	/// 0s everything and restarts
	stopwatch& restart();

	inline std::ostream& print( std::ostream& fp )  const
	{
		return fp << sec << "." << std::setfill('0') << std::setw(6) << msec;
	}
	double getTimeAsDouble() const
		{ return (double)sec + msec/1000000.0; }
	double calcTimeAsDouble(bool advance=false) 
		{ calcTime(advance); return (double)sec + msec/1000000.0; }
};
inline std::ostream& operator <<( std::ostream& fp, const stopwatch& sw )
{
	return sw.print(fp);
}

} // namesapce ay
