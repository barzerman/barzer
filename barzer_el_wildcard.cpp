#include <barzer_el_wildcard.h>
#include <barzer_el_trie.h>

namespace barzer {

std::ostream& BarzelWildcardPool::print( std::ostream& fp, const BarzelWCKey& key, const BELPrintContext& ctxt ) const
{
	uint32_t wcId = key.wcId;

	switch(key.wcType ) {
	case BTND_Pattern_Number_TYPE: 
	{
		const BTND_Pattern_Number* pat = pool_Number.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_Wildcard_TYPE: 
	{
		const BTND_Pattern_Wildcard* pat = pool_Wildcard.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_Date_TYPE: 
	{
		const BTND_Pattern_Date* pat = pool_Date.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_DateTime_TYPE: 
	{
		const BTND_Pattern_DateTime* pat = pool_DateTime.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_Time_TYPE: 
	{
		const BTND_Pattern_Time* pat = pool_Time.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_Range_TYPE: 
	{
		const BTND_Pattern_Range* pat = pool_Range.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_Entity_TYPE: 
	{
		const BTND_Pattern_Entity* pat = pool_Entity.getObjById( wcId );
		if( pat ) pat->print(fp,ctxt); else fp << "<null>";
	}
		break;
	case BTND_Pattern_ERCExpr_TYPE: 
	{
		const BTND_Pattern_ERCExpr* pat = pool_ERCExpr.getObjById( wcId );
		if( pat ) pat->print(fp); else fp << "<null>";
	}
		break;
	default: fp << "<undefined>";
	}
	return fp;
}

}
