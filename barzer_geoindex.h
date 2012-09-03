#pragma once

#include <algorithm>
#include <vector>

namespace barzer
{
template<typename PayloadT, typename Coord = double>
class Point
{
	Coord m_x, m_y;
	PayloadT m_payload;
public:
	Point(Coord x, Coord y, PayloadT payload = PayloadT())
	: m_x(x)
	, m_y(y)
	, m_payload(payload)
	{
	}
	
	Coord x() const { return m_x; }
	Coord y() const { return m_y; }

	const PayloadT& getPayload() const { return m_payload; }
	
	bool operator== (const Point& other) const
	{
		return m_x == other.m_x &&
				m_y == other.m_y &&
				m_payload == other.m_payload;
	}
	
	double operator- (const Point& other) const
	{
		const auto dx = m_x - other.m_x;
		const auto dy = m_y - other.m_y;
		return dx * dx + dy * dy;
	}
};

template<typename Point>
inline bool isXLess(const Point& p1, const Point& p2)
{
	return p1.x() < p2.x();
}

template<typename Point>
inline bool isYLess(const Point& p1, const Point& p2)
{
	return p1.y() < p2.y();
}

template<template<typename PayloadT, typename Coord> class PointT,
		typename PayloadT,
		typename Coord = double>
class GeoIndex
{
public:
	typedef PointT<PayloadT, Coord> Point;
	typedef std::vector<Point> Points_t;
	typedef Coord Coord_t;
private:
	Points_t m_points;
public:
	GeoIndex() {}
	
	void addPoint(const Point& point)
	{
		auto it = std::upper_bound (m_points.begin(), m_points.end(), point, isXLess<Point>);
		m_points.insert(it, point);
	}
	
	void setPoints(const Points_t& points)
	{
		m_points = points;
		std::sort(m_points.begin(), m_points.end(), isXLess<Point>);
	}
	
	template<typename CallbackT, typename PredT>
	void findPoints(const Point& center, CallbackT cb, PredT pred, Coord maxDist) const
	{
		auto upper = std::upper_bound(m_points.begin(), m_points.end(), center.x() + maxDist,
				[](Coord x, const Point& p) { return x < p.x(); });
		auto lower = std::lower_bound(m_points.begin(), upper, center.x() - maxDist,
				[](const Point& p, Coord x) { return p.x() < x; });

		Points_t sub;
		sub.reserve(upper - lower);
		std::copy_if(lower, upper, std::back_inserter (sub), pred);
		
		struct Sorter
		{
			const Point& m_p;
			Sorter(const Point& p)
			: m_p(p) {}
			
			bool operator()(const Point& left, const Point& right) const
			{
				return m_p - left < m_p - right;
			}
		};
		
		const auto powedDist = maxDist * maxDist;
		
		std::sort(sub.begin(), sub.end(), Sorter(center));
		for (typename Points_t::const_iterator i = sub.begin(); i != sub.end(); ++i)
			if (*i - center >= powedDist || !cb(*i))
				break;
	}

	template<typename CallbackT, typename PredT>
	void findPoints2(const Point& center, CallbackT cb, PredT pred, Coord maxDist) const
	{
		auto upper = std::upper_bound(m_points.begin(), m_points.end(), center.x() + maxDist,
				[](Coord x, const Point& p) { return x < p.x(); });
		auto lower = std::lower_bound(m_points.begin(), upper, center.x() - maxDist,
				[](const Point& p, Coord x) { return p.x() < x; });

		Points_t sub;
		sub.reserve(upper - lower);
		std::copy_if(lower, upper, std::back_inserter (sub), pred);
		
		struct Sorter
		{
			const Point& m_p;
			Sorter(const Point& p)
			: m_p(p) {}
			
			bool operator()(const Point& left, const Point& right) const
			{
				return m_p - left < m_p - right;
			}
		};
		
		std::sort(sub.begin(), sub.end(), isYLess<Point>);
		auto upper2 = std::upper_bound(sub.begin(), sub.end(), center.y() + maxDist,
				[](Coord y, const Point& p) { return y < p.y(); });
		auto lower2 = std::lower_bound(sub.begin(), upper2, center.y() - maxDist,
				[](const Point& p, Coord y) { return p.y() < y; });
		
		std::sort(lower2, upper2, Sorter(center));
		for (auto i = lower2; i != upper2; ++i)
			if (*i - center >= maxDist * maxDist || !cb(*i))
				break;
	}
};
}
