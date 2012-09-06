#include "barzer_geoindex.h"
#include <iostream>
#include <sys/time.h>

inline long getDiff(timeval prev, timeval now)
{
	return 1000000 * (now.tv_sec - prev.tv_sec) + now.tv_usec - prev.tv_usec;
}


using namespace barzer;

struct Callback
{
	template<typename T>
	bool operator()(const T& p)
	{
		//std::cout << "{ " << p.x() << "; " << p.y() << "}" << std::endl;
		return true;
	}
};

template<typename Vec, typename GI>
void test(const GI& gi, const Vec& points, typename GI::Coord_t dist)
{
	const int retries = 10;
	
	std::cout << "testing with distance " << dist << std::endl;
	
	timeval prev, now;	
	gettimeofday(&prev, 0);
	for (int i = 0; i < retries; ++i)
		for (const auto& pt : points)
			gi.findPoints(pt, Callback(), [](const typename GI::Point& p) { return true; }, dist);
	gettimeofday(&now, 0);
	
	std::cout << "badass approach:\t\t" << getDiff (prev, now) / retries / points.size() << " μs avg" << std::endl;
	std::cout << std::endl << std::endl;
}

namespace
{
	struct Payload
	{
		char m_data[32];
	};
}

int main()
{
	typedef GeoIndex<Point, Payload, double> GI;
	
	const int size = 3000;
	GI gi(3000);
	for (int i = 0; i < size; ++i)
		for (int j = 0; j < size; ++j)
			gi.addPoint(GI::Point(i, j));
		
	std::vector<GI::Point> testPts = {GI::Point(5, 5), GI::Point(5, 505), GI::Point(505, 5), GI::Point(505, 505), GI::Point(0, 0)};
	
	/*
	timeval prev, now;
	gettimeofday(&prev, 0);
	gi.findPoints(GI::Point(0, 5), Callback(), [](const GI::Point& p) { return true; }, 2);
	gettimeofday(&now, 0);
	std::cout << getDiff(prev, now) << std::endl;
	
	return 0;
	*/
	
	test(gi, testPts, 2);
	test(gi, testPts, 5);
	test(gi, testPts, 20);
	test(gi, testPts, 30);
	test(gi, testPts, 50);
	test(gi, testPts, 100);
}