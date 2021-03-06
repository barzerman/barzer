
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <boost/unordered_map.hpp>
#include <ay/ay_geo.h>
#include <barzer_storage_types.h>
#include "barzer_universe.h"

namespace barzer
{
typedef ay::geo::GeoIndex<ay::geo::Point, StoredEntityId> GeoIndex_t;

class SerializingCB
{
	std::vector<uint32_t>& m_out;
	size_t m_counter;
	const size_t m_max;
public:
	SerializingCB(std::vector<uint32_t>& out, size_t num)
	: m_out(out)
	, m_counter(0)
	, m_max(num)
	{
	}
	
	bool operator()(const GeoIndex_t::Point& p)
	{
		m_out.push_back(p.getPayload());
		return ++m_counter < m_max;
	}
};

class BarzerGeo
{
	GeoIndex_t m_idx;
	boost::unordered_map<StoredEntityId, GeoIndex_t::Point> m_entity2point;
public:
	typedef GeoIndex_t::Point Point_t;
	typedef GeoIndex_t::Coord_t Coord_t;
	
	BarzerGeo();
	
	void addEntity(const StoredEntity&, const std::pair<double, double>&);
	
	template<typename Pred>
	void findEntities(std::vector<uint32_t>& out, const Point_t& center,
			const Pred& pred, GeoIndex_t::Coord_t dist, size_t num = std::string::npos) const
	{
		return m_idx.findPoints(center, SerializingCB(out, num), pred, dist);
	}
	
	void proximityFilter(std::vector<StoredEntityId>& ents,
			const Point_t& center, GeoIndex_t::Coord_t dist, bool sorted) const;
	/** Returns true if the entity identified by singleEnt passes the proximity
	 * filter.
	 */
	bool proximityFilter(const StoredEntityId& singleEnt,
			const Point_t& center, GeoIndex_t::Coord_t dist) const;
    void clear()
        { 
            m_entity2point.clear(); 
            m_idx.clear();
        }
    
    /// longitude GeoIndex_t::Point::x() lattitude GeoIndex_t::Point::y() 
    const GeoIndex_t::Point* getCoords( const StoredEntityId& ent ) const
    {
        auto i= m_entity2point.find( ent );
        return( i == m_entity2point.end() ? 0: &(i->second) );
    }
};

struct FilterParams
{
	bool m_valid;
	BarzerGeo::Point_t m_point;
	BarzerGeo::Coord_t m_dist;
	std::vector<uint32_t> m_subclasses;
	
	FilterParams() : m_valid(false) {}

	FilterParams(const BarzerGeo::Point_t& point, BarzerGeo::Coord_t dist, const std::vector<uint32_t>& sc)
	: m_valid(true)
	, m_point(point)
	, m_dist(dist)
	, m_subclasses(sc)
	{
	}
	
	static FilterParams fromBarz(const Barz& barz);
	static FilterParams fromExtraMap(const std::map<std::string, std::string>&);
};

void proximityFilter(Barz& barz, const StoredUniverse& uni);
void proximityFilter(Barz& barz, const StoredUniverse& uni, const FilterParams&);

class DumbPred
{
public:
	bool operator()(const BarzerGeo::Point_t&) const { return true; }
};

class EntClassPred
{
	const uint32_t m_ec;
	const uint32_t m_esc;
	const StoredUniverse& m_uni;
public:
	EntClassPred(uint32_t ec, uint32_t esc, const StoredUniverse& uni)
	: m_ec(ec)
	, m_esc(esc)
	, m_uni(uni)
	{
	}
	
	bool operator()(const BarzerGeo::Point_t& p) const
	{
		const auto entId = p.getPayload();
		const auto ent = m_uni.getGlobalPools().getDtaIdx().getEntById(entId);
		return ent &&
				ent->getClass() == m_ec &&
				ent->getSubclass() == m_esc;
	}
};
} // namespace barzer
