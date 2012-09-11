#pragma once

#include <boost/unordered_map.hpp>
#include <ay/ay_geo.h>
#include <barzer_storage_types.h>

namespace barzer
{
class BarzerGeo
{
	typedef ay::geo::GeoIndex<ay::geo::Point, StoredEntityId> GeoIndex_t;
	GeoIndex_t m_idx;
public:
	BarzerGeo();
	
	void addEntity(const StoredEntity&, const std::pair<double, double>&);
};
}
