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
inline bool isXLess(const Point& p1, const Point& p2) { return p1.x() < p2.x(); }

template<typename Point, typename Coord>
inline bool isXScalarLess(Coord x, const Point& p) { return x < p.x(); }

template<typename Point, typename Coord>
inline bool isXLessScalar(const Point& p, Coord x) { return p.x() < x; }

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
	Points_t m_xpoints;
public:
	GeoIndex() {}
	
	void addPoint(const Point& point)
	{
		auto xit = std::upper_bound (m_xpoints.begin(), m_xpoints.end(), point, isXLess<Point>);
		m_xpoints.insert(xit, point);
	}
	
	void setPoints(const Points_t& points)
	{
		m_xpoints = points;
		std::sort(m_xpoints.begin(), m_xpoints.end(), isXLess<Point>);
	}
	
	template<typename CallbackT, typename PredT>
	void findPoints(const Point& center, CallbackT cb, PredT pred, Coord maxDist) const
	{
		auto upper = std::upper_bound(m_xpoints.begin(), m_xpoints.end(), center.x() + maxDist, isXScalarLess<Point, Coord>);
		auto lower = std::lower_bound(m_xpoints.begin(), upper, center.x() - maxDist, isXLessScalar<Point, Coord>);

		Points_t sub;
		sub.reserve(upper - lower);
		auto upY = center.y() + maxDist;
		auto downY = center.y() - maxDist;
		
		struct CopyPred
		{
			Coord m_downY;
			Coord m_upY;
			
			PredT m_pred;
		public:
			CopyPred (Coord down, Coord up, PredT pred)
			: m_downY(down)
			, m_upY(up)
			, m_pred(pred)
			{
			}
			
			bool operator()(const Point& p) const
			{
				return p.y() < m_upY && p.y() >= m_downY && m_pred(p);
			}
		};
		
		std::copy_if(lower, upper, std::back_inserter (sub), CopyPred(downY, upY, pred));
		
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
		for (auto i = sub.begin(); i != sub.end(); ++i)
			if (*i - center >= powedDist || !cb(*i))
				break;
	}
};
}
