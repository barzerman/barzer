#include "barzer_geoindex.h"
#include "barzer_server.h"
#include "barzer_el_cast.h"

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
	std::vector<StoredEntityId> geolessEnts;
	std::vector<Point_t> points;
	for (std::vector<StoredEntityId>::const_iterator i = ents.begin(), end = ents.end(); i != end; ++i)
	{
		auto pos = m_entity2point.find(*i);
		if (pos != m_entity2point.end())
			points.push_back(pos->second);
		else
			geolessEnts.push_back(*i);
	}

	GeoIndex_t tmpIdx(WrapAround);
	tmpIdx.setPoints(points);

	ents.clear();
	tmpIdx.findPoints(center, makePushingCB(ents), DumbPred(), dist, sorted);

	std::copy(geolessEnts.begin(), geolessEnts.end(), std::back_inserter(ents));
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
		BarzerNumber num;
		return BarzerAtomicCast().convert(num, boost::get<BarzerString>(*var)) == BarzerAtomicCast::CASTERR_OK ?
				num.getRealWiden() :
				-1;
	}

	double GetDist(const BarzelBeadAtomic_var *distVar, const BarzelBeadAtomic_var *unitVar)
	{
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
		return dist;
	}

	std::vector<uint32_t> ParseEntSC(const BarzelBeadAtomic_var *escVar)
	{
		std::vector<uint32_t> result;
		if (!escVar || escVar->which() != BarzerString_TYPE)
			return result;

		const auto& str = boost::get<BarzerString>(*escVar).getStr();
		std::istringstream istr(str);
		while (istr.good())
		{
			uint32_t sc = 0;
			istr >> sc;
			result.push_back(sc);
		}
		return result;
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
		const std::vector<uint32_t>& m_subclasses;
	public:
		EntityFilterVisitor(const StoredUniverse& uni,
				const GeoIndex_t::Point& center,
				GeoIndex_t::Coord_t dist,
				const std::vector<uint32_t>& subclasses)
		: m_uni(uni)
		, m_center(center)
		, m_dist(dist)
		, m_subclasses(subclasses)
		{
		}

		FilterResult operator()(BarzerEntity& e) const
		{
			auto se = m_uni.getDtaIdx().getEntByEuid(e);
			if (!se)
				return FilterResult();

			if (!subclassBelongs(se))
				return FilterResult();

			return FilterResult(false, !m_uni.getGeo()->proximityFilter(se->entId, m_center, m_dist));
		}

		FilterResult operator()(BarzerEntityList& e) const
		{
			const auto& dta = m_uni.getDtaIdx();

			std::vector<StoredEntityId> ents, toPreserve;
			ents.reserve(e.size());
			auto& list = e.theList();
			for (auto i = list.begin(); i != list.end(); ++i)
			{
				auto se = dta.getEntByEuid(*i);
				if (!se)
					continue;

				if (subclassBelongs(se))
					ents.push_back(se->entId);
				else
					toPreserve.push_back(se->entId);
			}

			m_uni.getGeo()->proximityFilter(ents, m_center, m_dist, false);
			std::copy(toPreserve.begin(), toPreserve.end(), std::back_inserter(ents));
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
	private:
		bool subclassBelongs(const StoredEntity* ent) const
		{
			return m_subclasses.empty() ||
					std::find(m_subclasses.begin(), m_subclasses.end(), ent->getSubclass()) != m_subclasses.end();
		}
	};
}

FilterParams FilterParams::fromBarz(const Barz& barz)
{
	const auto reqMap = barz.getRequestVariableMap();
	if (!reqMap)
		return FilterParams();

	auto lonVar = reqMap->getValue("geo::lon");
	auto latVar = reqMap->getValue("geo::lat");
	auto distVar = reqMap->getValue("geo::dist");
	if (!lonVar || !latVar || !distVar ||
			lonVar->which() != BarzerString_TYPE ||
			latVar->which () != BarzerString_TYPE ||
			distVar->which () != BarzerString_TYPE)
		return FilterParams();

	return FilterParams (GeoIndex_t::Point(String2Double(lonVar), String2Double(latVar)),
			GetDist(distVar, reqMap->getValue("geo::dunit")),
			ParseEntSC(reqMap->getValue("geo::subclasses")));
}

void proximityFilter(Barz& barz, const StoredUniverse& uni)
{
	const auto& params = FilterParams::fromBarz(barz);
	if (!params.m_valid)
		return;

	BarzelBeadChain& bc = barz.getBeads();
	const EntityFilterVisitor filterVis(uni, params.m_point, params.m_dist, params.m_subclasses);

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
