#include "barzer_geoindex.h"

namespace barzer
{
BarzerGeo::BarzerGeo()
: m_idx(360)
{
}

void BarzerGeo::addEntity (const StoredEntity& entity, const std::pair<double, double>& coords)
{
	m_idx.addPoint(GeoIndex_t::Point(coords.first, coords.second, entity.entId));
}
}
