#include <barzer_el_pattern_number.h>
#define TAG_CLOSE_STR "/>" 

namespace barzer {

std::ostream&  BTND_Pattern_Number::print( std::ostream& fp, const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_NUMBER:
		fp << "AnyNum";
		break;
	case T_ANY_INT:
		fp << "AnyInt";
		break;
	case T_ANY_REAL:
		fp << "AnyReal";
		break;
	case T_RANGE_INT:
		fp << "RangeInt[" << range.integer.lo << "," << range.integer.hi << "]";
		break;
	case T_RANGE_REAL:
		fp << "RangeReal[" << range.real.lo << "," << range.real.hi << "]";
		break;
	default:
		return ( fp << "NumUnknown" );	
	}
	return fp; 
}

std::ostream& BTND_Pattern_Number::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<n";
	if( d_asciiLen ) {
		fp << " w=\"" <<  d_asciiLen << '"';
	}
	switch( type ) {
	case T_ANY_INT: break;
	case T_ANY_REAL:
		fp << " r =\"y\"";
		break;
	case T_RANGE_INT: 
		fp << " l=\"" << range.integer.lo << "\" h=\"" << range.integer.hi << "\""; break;
	case T_RANGE_REAL: 
		fp << " l=\"" << range.real.lo << "\" h=\"" << range.real.hi << "\""; break;
	case T_ANY_NUMBER: break;
	}
	return fp << TAG_CLOSE_STR;
}
bool BTND_Pattern_Number::operator() ( const BarzerNumber& num ) const
{
	uint8_t len = getAsciiLen();
	if(len && len!= num.getAsciiLen()) 
		return false;
		
	switch( type ) {
	case T_ANY_NUMBER: return true;
	case T_ANY_INT: return num.isInt();
	case T_ANY_REAL: return true;

	case T_RANGE_INT:
		if( num.isInt() ) {
			int n = num.getInt_unsafe();
            if( rangeType == RT_FULL ) 
			    return( n <= range.integer.hi && n>=range.integer.lo );
            else if( rangeType == RT_NOHI ) 
			    return( n>=range.integer.lo );
            else if( rangeType == RT_NOLO ) 
			    return( n <= range.integer.hi );
            else
                return true;
		} else 
			return false;
	case T_RANGE_REAL:
        {
			double n =  num.getReal();

            if( rangeType == RT_FULL ) 
			    return( n <= range.real.hi && n>=range.real.lo );
            else if( rangeType == RT_NOHI ) 
			    return( n>=range.real.lo );
            else if( rangeType == RT_NOLO ) 
			    return( n <= range.real.hi );
            else
                return true;
        }
		break;
	default: 
		return false;
	}
	return false;
}

} // namespace barzer 
