#include <ay_util_time.h>
#include <stdio.h>
#include <stdlib.h>

namespace ay {
stopwatch::stopwatch( ) : sec(0), msec(0)
{
	gettimeofday(&tmpTv, 0 );
}
stopwatch& stopwatch::start( )
{
	gettimeofday(&tmpTv, 0 );
	return *this;
}
stopwatch& stopwatch::calcTime(bool advance )
{
	struct timeval tv;
	gettimeofday(&tv, 0 );
	//std::cerr << "CRAP: " << tv.tv_sec << ":" << tv.tv_usec << " vs " << 
	//tmpTv.tv_sec << ":" << tmpTv.tv_usec << "\n";
	
	time_t tmpSec = sec;
	suseconds_t tmpMSec = msec;

	if( tv.tv_usec< tmpTv.tv_usec ) {
		msec = 1000000 + tv.tv_usec - tmpTv.tv_usec;
		sec = tv.tv_sec - tmpTv.tv_sec-1;
	} else {
		msec = tv.tv_usec - tmpTv.tv_usec;
		sec = tv.tv_sec - tmpTv.tv_sec;
	}
	if( advance ) {
		sec += tmpSec;
		msec += tmpMSec;
	}
	if( msec >= 1000000 ) {
		++sec;
		msec -= 1000000;
	}
	return *this;
}

stopwatch& stopwatch::restart( )
{
	gettimeofday(&tmpTv, 0 );

	msec = 0;
	sec = 0;
	return *this;
}

} // namespace ay

#ifdef AY_UTIL_TIME_CPP_TEST
main( int argc, char* argv[] )
{
	ay::stopwatch sw;
	ay::stopwatch sw1;
	int iter = 0;
	while(1) {
		sw.start();
		for( int i=0; i< 6000000; ++i ) ;
		sw.stop();
		std::cerr << "timer 1 " <<  sw<< "\n";
		sleep(1);
		std::cerr << ++iter << "  timer 2 " <<  sw1.calcTime() << "\n";
	}
	getchar();	
	std::cerr << "elapsed sec:microsec " <<  sw.stop()<< "\n";
}
#endif // AY_UTIL_TIME_CPP_TEST
