#include <barzer_el_pattern_datetime.h>

namespace barzer {

std::ostream&  BTND_Pattern_Date::print( std::ostream& fp , const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_DATE:
		return (fp << "AnyDate");
	case T_ANY_FUTUREDATE:
		return (fp << "AnyFutureDate");
	case T_ANY_PASTDATE:
		return (fp << "AnyPastDate");

	case T_DATERANGE:
		return ( fp << "Date[" << lo << "," << hi << "]" );
	}
	return (fp << "UnknownDate" );
}
std::ostream&  BTND_Pattern_Time::print( std::ostream& fp , const BELPrintContext& ) const
{

	switch( type ) {
	case T_ANY_TIME: return( fp << "AnyTime" );
	case T_ANY_FUTURE_TIME: return( fp << "AnyFutureTime" );
	case T_ANY_PAST_TIME:return( fp << "AnyPastTime" );
	case T_TIMERANGE:return( fp << "Time["  << lo << "," << hi << "]");
	}
	return ( fp << "UnknownTime" );
}
std::ostream&  BTND_Pattern_DateTime::print( std::ostream& fp , const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_DATETIME: return (fp << "AnyDateTime" );
	case T_ANY_FUTURE_DATETIME: return (fp << "FutureDateTime" );
	case T_ANY_PAST_DATETIME: return (fp << "PastDateTime" );
	case T_DATETIME_RANGE: return (fp << "DateTime[" << lo << "-" << hi << "]");
	}
	return (fp << "DateTimeUnknown");
}

std::ostream& BTND_Pattern_Time::printXML( std::ostream& fp, const GlobalPools& gp ) const 
{
	fp << "<time";
	switch(type) {
	case T_ANY_TIME:
		break;
	case T_ANY_FUTURE_TIME:
		fp << " f=\"y\"";
		break;
	case T_ANY_PAST_TIME: 
		fp << " p=\"y\"";
		break;
	case T_TIMERANGE: 
		fp << " l=\"" << getLoLong() << "\" h=\"" << getHiLong() << "\"";
		break;
	}
	fp << "/>";
	return fp;
}
std::ostream& BTND_Pattern_Date::printXML( std::ostream& fp, const GlobalPools& gp ) const 
{
	fp << "<date";
	switch( type ) {
	case T_ANY_DATE:
		break;
	case T_ANY_FUTUREDATE:
		fp << " f=\"y\"";
		break;
	case T_ANY_PASTDATE:
		fp << " p=\"y\"";
		break;
	case T_TODAY: // this seems to be unused
		break;

	case T_DATERANGE:
		if( isLoSet() ) {
			fp << " l=\"" << lo << "\"";
		}
		if( isHiSet() ) {
			fp << " h=\"" << hi << "\"";
		}
		break;
	}
	fp << "/>";

	return fp;
}

std::ostream& BTND_Pattern_DateTime::printXML( std::ostream& fp, const GlobalPools& gp ) const 
{
	fp << "<dtim";
	switch(type) {
	case T_ANY_DATETIME:
		break;
	case T_ANY_FUTURE_DATETIME: 
		fp << " f=\"y\"";
		break;
	case T_ANY_PAST_DATETIME: 
		fp << " p=\"y\"";
		break;
	case T_DATETIME_RANGE: 
		if( lo.isValid() ) {
			if( lo.hasDate() ) {
				fp << " dl=\"" << lo.date.getLong() << "\"";
			}
			if( lo.hasTime() ) {
				fp << " tl=\"" << lo.timeOfDay.getLong() << "\"";
			}
		}
		if( hi.isValid() ) {
			if( hi.hasDate() ) {
				fp << " dh=\"" << hi.date.getLong() << "\"";
			}
			if( hi.hasTime() ) {
				fp << " th=\"" << hi.timeOfDay.getLong() << "\"";
			}
		}
		break;
	}
	fp << "/>";
	return fp;
}
} // namespace barzer
