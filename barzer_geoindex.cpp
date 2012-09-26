#include "barzer_geoindex.h"
#include "barzer_server.h"
#include <boost/lexical_cast.hpp>

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

void BarzerGeo::proximityFilter(std::vector<StoredEntityId>& ents, const Point_t& center, GeoIndex_t::Coord_t dist, bool sorted) const
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

bool BarzerGeo::proximityFilter(const StoredEntityId& singleEnt, const Point_t& center, GeoIndex_t::Coord_t dist) const
{
	std::vector<StoredEntityId> vec;
	vec.push_back(singleEnt);
	proximityFilter(vec, center, dist, false);
	return !vec.empty();
}

namespace
{
	double String2Double(const BarzelBeadAtomic_var *var)
	{
		const auto& string = boost::get<BarzerString>(*var);
		try
		{
			return boost::lexical_cast<double>(string.getStr());
		}
		catch (...)
		{
			return - 1;
		}
	}
	
	struct FilterResult
	{
		bool updated;
		bool shouldRemove;
		
		FilterResult()
		: updated(false)
		, shouldRemove(false)
		{
		}
		
		FilterResult(bool upd, bool rem)
		: updated(upd)
		, shouldRemove(rem)
		{
		}
	};
	
	class EntityFilterVisitor : public boost::static_visitor<FilterResult>
	{
		const StoredUniverse& m_uni;
		const GeoIndex_t::Point& m_center;
		GeoIndex_t::Coord_t m_dist;
	public:
		EntityFilterVisitor(const StoredUniverse& uni, const GeoIndex_t::Point& center, GeoIndex_t::Coord_t dist)
		: m_uni(uni)
		, m_center(center)
		, m_dist(dist)
		{
		}
		
		FilterResult operator()(BarzerEntity& e) const
		{
			auto se = m_uni.getDtaIdx().getEntByEuid(e);
			return se ?
					FilterResult(false, !m_uni.getGeo()->proximityFilter(se->entId, m_center, m_dist)) :
					FilterResult();
		}
		
		FilterResult operator()(BarzerEntityList& e) const
		{
			const auto& dta = m_uni.getDtaIdx();
			
			std::vector<StoredEntityId> ents;
			ents.reserve(e.size());
			auto& list = e.theList();
			for (auto i = list.begin(); i != list.end(); ++i)
			{
				auto se = dta.getEntByEuid(*i);
				if (se)
					ents.push_back(se->entId);
			}
			
			m_uni.getGeo()->proximityFilter(ents, m_center, m_dist, false);
			if (ents.size() == e.size())
				return FilterResult(false, false);
			
			if (ents.empty())
				return FilterResult(false, true);
			
			auto pos = list.begin();
			while (pos != list.end())
			{
				auto se = dta.getEntByEuid(*pos);
				if (!se)
				{
					++pos;
					continue;
				}
				
				if (std::find(ents.begin(), ents.end(), se->entId) == ents.end())
					pos = list.erase(pos);
				else
					++pos;
			}
			return FilterResult(true, false);
		}
		
		template<typename T>
		FilterResult operator()(T&) const
		{
			return FilterResult();
		}
	};
}

void proximityFilter(Barz& barz, const StoredUniverse& uni)
{
	const auto reqMap = barz.getRequestVariableMap();
	if (!reqMap)
		return;
	
	auto lonVar = reqMap->getValue("geo::lon");
	auto latVar = reqMap->getValue("geo::lat");
	auto distVar = reqMap->getValue("geo::dist");
	auto unitVar = reqMap->getValue("geo::dunit");
	if (!lonVar || !latVar || !distVar ||
			lonVar->which() != BarzerString_TYPE ||
			latVar->which () != BarzerString_TYPE ||
			distVar->which () != BarzerString_TYPE)
		return;
	
	const double lon = String2Double(lonVar);
	const double lat = String2Double(latVar);
	double dist = String2Double(distVar);
	
	auto unit = ay::geo::Unit::Kilometre;
	if (unitVar && unitVar->which() == BarzerString_TYPE)
	{
		const auto& unitStr = boost::get<BarzerString>(*unitVar).getStr();
		const auto parsedUnit = ay::geo::unitFromString(unitStr);
		if (parsedUnit < ay::geo::Unit::MAX)
			unit = parsedUnit;
	}
	
	dist = ay::geo::convertUnit(dist, unit, ay::geo::Unit::Degree);
	
	BarzelBeadChain& bc = barz.getBeads();
	const EntityFilterVisitor filterVis(uni, GeoIndex_t::Point(lon, lat), dist);
	
	auto bli = bc.getLstBegin();
	while (bc.isIterNotEnd(bli))
	{
		auto data = bli->getBeadData();
		if (data.which() != BarzelBeadAtomic_TYPE)
		{
			++bli;
			continue;
		}
		
		auto atomic = boost::get<BarzelBeadAtomic>(data).dta;
		const auto& result = boost::apply_visitor(filterVis, atomic);
		std::cout << "result " << result.shouldRemove << result.updated << std::endl;
		if (result.shouldRemove)
			bli = bc.lst.erase(bli);
		else
		{
			if (result.updated)
			{
				data = BarzelBeadAtomic(atomic);
				bli->setData(data);
			}
			++bli;
		}
	}
}
}
