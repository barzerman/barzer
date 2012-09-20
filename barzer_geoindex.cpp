#include "barzer_geoindex.h"

namespace barzer
{
const GeoIndex_t::Coord_t WrapAround = 360;

BarzerGeo::BarzerGeo()
: m_idx(WrapAround)
{
}

void BarzerGeo::addEntity (const StoredEntity& entity, const std::pair<double, double>& coords)
{
	const Point_t point(coords.first, coords.second, entity.entId);
	m_idx.addPoint(point);
	m_entity2point.insert(std::make_pair(entity.entId, point));
}

namespace
{
	template<typename Cont>
	struct PushingCallback
	{
		Cont& m_container;
	public:
		PushingCallback(Cont& c)
		: m_container(c)
		{
		}
		
		bool operator()(const GeoIndex_t::Point& val)
		{
			m_container.push_back(val.getPayload());
			return false;
		}
	};
	
	template<typename Cont>
	PushingCallback<Cont> makePushingCB(Cont& cont)
	{
		return PushingCallback<Cont>(cont);
	}
}

void BarzerGeo::proximityFilter (std::vector<StoredEntityId>& ents, const Point_t& center, GeoIndex_t::Coord_t dist, bool sorted) const
{
	std::vector<Point_t> points;
	for (std::vector<StoredEntityId>::const_iterator i = ents.begin(), end = ents.end(); i != end; ++i)
	{
		auto pos = m_entity2point.find(*i);
		if (pos != m_entity2point.end())
			points.push_back(pos->second);
	}
	
	GeoIndex_t tmpIdx(WrapAround);
	tmpIdx.setPoints(points);
	
	ents.clear();
	tmpIdx.findPoints(center, makePushingCB(ents), DumbPred(), dist, sorted);
}
}
