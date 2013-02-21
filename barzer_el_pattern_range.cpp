#include <barzer_el_pattern_range.h>
#include <barzer_universe.h>

namespace barzer {

namespace
{
	inline bool rangeTypeMatches (uint32_t dType, uint32_t eType)
	{
		if (dType == eType)
			return true;

		if (dType == BarzerRange::Real_TYPE && eType == BarzerRange::Integer_TYPE)
			return true;

		return false;
	}
}

bool BTND_Pattern_Range::operator() (const BarzerRange& e) const
{
	if( d_rangeFlavor == FLAVOR_NUMERIC ) {
		if( !d_range.isNumeric() || !e.isNumeric() )
			return false;

		if( d_mode == MODE_TYPE ) {
			return true;
		}
		if( d_mode == MODE_VAL ) {
			if( d_range.isReal() ) {
				if( e.isReal() )
					return (d_range == e);
				else {
					BarzerRange eR(e);
					return ( eR.promote_toReal() == d_range );
				}
			} else
			if( d_range.isInteger() ) {
				if( e.isInteger() )
					return (d_range == e);
				else {
					// e is a real range
					BarzerRange eR(d_range);
					return ( eR.promote_toReal() == e );
				}
			} else
				return false;
		} else
			return false;
	} else {
		if( d_mode == MODE_TYPE ) {
			return rangeTypeMatches (d_range.getType(), e.getType()) || d_range.isNone();
		} else if( d_mode == MODE_VAL ) {
			if( d_range.getType() == e.getType() ) {
				if( !e.isEntity() ) {
					return ( d_range == e );
				} else {
					const BarzerRange::Entity* otherEntPair = e.getEntity();
					const BarzerRange::Entity* thisEntPair = d_range.getEntity();
					return (
						thisEntPair->first.matchOther( otherEntPair->first ) &&
						thisEntPair->second.matchOther( otherEntPair->second )
					);
				}
			} else
				return false;
		} else
			return false;
	}
}

std::ostream& BTND_Pattern_Range::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<range";
	if( range().isValid() ) 
		fp << " t=\"" << range().geXMLtAttrValueChar()  << "\"";
	if( isModeVal() ) {
		fp << " m=\"v\"";
	}
	const BarzerRange::Entity* re = d_range.getEntity();
	if( re ) {
		fp << " ec=\"" << re->first.eclass.ec << "\" es=\"" << re->first.eclass.subclass << "\"";
		if( re->first.tokId != 0xffffffff ) {
			fp << " ei1=\"" << gp.decodeStringById_safe(re->first.tokId ) << "\"";
		}
		if( re->second.tokId != 0xffffffff ) {
			fp << " ei1=\"" << gp.decodeStringById_safe(re->second.tokId ) << "\"";
		}
	}
	return fp << "/>";
}
} // namespace barzer
