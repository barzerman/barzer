#include <barzer_el_pattern_entity.h> 
#include <barzer_universe.h> 

namespace barzer {


void BTND_Pattern_Entity::setRange() {
    if ( d_rangeIsValid == 0 ) d_rangeIsValid= 1;
}


std::ostream& BTND_Pattern_Entity::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<ent";
	if( d_ent.eclass.ec ) {
		fp << " c=\"" << d_ent.eclass.ec << "\"";
	}
	if( d_ent.eclass.subclass ) {
		fp << " s=\"" << d_ent.eclass.subclass << "\"";
	}
	if( d_ent.tokId != 0xffffffff ) {
        if( auto x = gp.internalString_resolve(d_ent.tokId) ) {
            xmlEscape( x, fp<< " t=\"" ) << "\"";
        } 
	}
	fp << "/>";
	return fp;
}

std::ostream& BTND_Pattern_ERCExpr::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<ercexpr";
	return fp << "/>";
}

std::ostream& BTND_Pattern_ERC::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<erc";
	if( isRangeValid() ) {
		fp << " r=\"" << d_erc.getRange().geXMLtAttrValueChar() << "\"";
	} else if(isMatchBlankRange()) {
		fp << " br=\"y\"";
	}
	if( isEntityValid() ) {
		fp << " c=\"" << d_erc.getEntity().eclass.ec << "\"";
		if( d_erc.getEntity().eclass.subclass ) {
			fp << " s=\"" << d_erc.getEntity().eclass.subclass << "\"";
		}
		if( d_erc.getEntity().tokId != 0xffffffff ) {
			fp << " t=\"" << gp.decodeStringById_safe( d_erc.getEntity().tokId ) << "\"";
		}
	} else if(isMatchBlankEntity()){
		fp << " be=\"y\"";
	}
	if( isUnitEntityValid() ) {
		fp << " uc=\"" << d_erc.getUnitEntity().eclass.ec << "\"";
		if( d_erc.getEntity().eclass.subclass ) {
			fp << " us=\"" << d_erc.getUnitEntity().eclass.subclass << "\"";
		}
		if( d_erc.getEntity().tokId != 0xffffffff ) {
			fp << " ut=\"" << gp.decodeStringById_safe( d_erc.getUnitEntity().tokId ) << "\"";
		}
	}

	return fp << "/>";
}

} // namespace barzer
