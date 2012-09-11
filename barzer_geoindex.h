#pragma once

#include <algorithm>
#include <vector>
#include <iostream>

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
	
	void setX(Coord x) { m_x = x; }
	void setY(Coord y) { m_y = y; }

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

template<typename Point, typename Coord>
inline double wrapDist(const Point& p1, const Point& p2, Coord wrapAround)
{
	auto dumbDiff = p1 - p2;
	if (!wrapAround)
		return dumbDiff;
	
	const bool isFirstLeft = isXLess(p1, p2);
	
	auto tmpPoint = isFirstLeft ? p1 : p2;
	const auto& right = isFirstLeft ? p2 : p1;
	
	tmpPoint.setX(tmpPoint.x() + wrapAround);
	
	return std::min(dumbDiff, tmpPoint - right);
}

template<template<typename PayloadT, typename Coord> class PointT,
		typename PayloadT,
		typename Coord = double>
class GeoIndex
{
	const Coord m_wrapAround;
public:
	typedef PointT<PayloadT, Coord> Point;
	typedef std::vector<Point> Points_t;
	typedef Coord Coord_t;
private:
	Points_t m_xpoints;
public:
	GeoIndex(Coord wrapAround = 0) : m_wrapAround(wrapAround) {}
	
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
		Points_t sub;
		
		const auto upY = center.y() + maxDist;
		const auto downY = center.y() - maxDist;
		
		struct CopyPred
		{
			const Coord m_downY;
			const Coord m_upY;
			
			const PredT m_pred;
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
		} copyPred(downY, upY, pred);
		
		const auto downX = center.x() - maxDist;
		const auto upX = center.x() + maxDist;
		
		const bool shouldWrap = m_wrapAround && (downX < 0 || upX > m_wrapAround);
		if (!shouldWrap)
			addPointsInDist(sub, downX, upX, copyPred);
		else if (downX < 0)
		{
			addPointsInDist(sub, 0, upX, copyPred);
			addPointsInDist(sub, m_wrapAround + downX, m_wrapAround, copyPred);
		}
		else if (upX > m_wrapAround)
		{
			addPointsInDist(sub, downX, m_wrapAround, copyPred);
			addPointsInDist(sub, 0, upX - m_wrapAround, copyPred);
		}
		
		struct Sorter
		{
			const Point& m_p;
			Coord m_wrap;
			Sorter(const Point& p, Coord wrap)
			: m_p(p)
			, m_wrap(wrap)
			{}
			
			bool operator()(const Point& left, const Point& right) const
			{
				return wrapDist(m_p, left, m_wrap) < wrapDist(m_p, right, m_wrap);
			}
		};
		std::sort(sub.begin(), sub.end(), Sorter(center, shouldWrap ? m_wrapAround : 0));
		
		const auto powedDist = maxDist * maxDist;		
		for (auto i = sub.begin(); i != sub.end(); ++i)
			if (wrapDist(*i, center, shouldWrap ? m_wrapAround : 0) >= powedDist || !cb(*i))
				break;
	}
private:
	template<typename PredT>
	void addPointsInDist(Points_t& out, Coord from, Coord to, PredT pred) const
	{
		auto upper = std::upper_bound(m_xpoints.begin(), m_xpoints.end(), to, isXScalarLess<Point, Coord>);
		auto lower = std::lower_bound(m_xpoints.begin(), upper, from, isXLessScalar<Point, Coord>);

		out.reserve(out.size() + (upper - lower));
		std::copy_if(lower, upper, std::back_inserter(out), pred);
	}
};
}
