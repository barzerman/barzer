#pragma once

#include <boost/unordered_map.hpp>
#include <ay/ay_geo.h>
#include <barzer_storage_types.h>
#include "barzer_universe.h"

namespace barzer
{
typedef ay::geo::GeoIndex<ay::geo::Point, StoredEntityId> GeoIndex_t;

class BarzerGeo
{
	GeoIndex_t m_idx;
public:
	typedef GeoIndex_t::Point Point_t;
	
	BarzerGeo();
	
	void addEntity(const StoredEntity&, const std::pair<double, double>&);
	
	template<typename Pred>
	void findEntities(std::vector<uint32_t>& out, const Point_t& center,
			const Pred& pred, size_t dist, size_t num = std::string::npos) const
	{
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
			
			bool operator()(const Point_t& p)
			{
				m_out.push_back(p.getPayload());
				return ++m_counter < m_max;
			}
		};
		
		return m_idx.findPoints(center, SerializingCB(out, num), pred, dist);
	}
};

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
}
