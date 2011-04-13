#include <barzer_basic_types.h>
#include <time.h>

namespace barzer {

uint8_t BarzerDate::thisMonth = 0;
uint8_t BarzerDate::thisDay = 0;
int16_t BarzerDate::thisYear =BarzerDate::INVALID_YEAR;

/// must be called first thing in the program
void BarzerDate::initToday()
{
	time_t t = time(0);
	struct tm theTm = {0};
    localtime_r(&t, & theTm);
	thisMonth = theTm.tm_mon+1;
	thisDay = theTm.tm_mday;
	thisYear = theTm.tm_year;
}

/// will try to set year to current year 
void BarzerDate::setDayMonth(const BarzerNumber& d, const BarzerNumber& m) 
{
	month = m.getInt();
	day = d.getInt();
	year = thisYear;
}

std::ostream& BarzerEntityList::print( std::ostream& fp ) const
{
	for( EList::const_iterator i = entList.begin(); i!= entList.end(); ++i ) {
		fp << '{' << *i << "}";
	}
	return fp;
}

namespace {

static const char* g_BarzerLiteral_typeName[] = {
		"STRING",
		"COMPOUND",
		"STOP",
		"PUNCT",
		"BLANK"
};

}

static const char* BarzerLiteral::getTypeName(int t) const
{
	return( ARR_NONNULL_STR( g_BarzerLiteral_typeName, t) ) ;
}

std::ostream& BarzerLiteral::print( std::ostream& fp ) const
{
	return ( fp << getTypeName(type) << ":" << std::hex << theId );
}

std::ostream& BarzerLiteral::print( std::ostream& fp, const BELPrintContext& ) const
{
	return print( fp );
}
std::ostream& BarzerLiteral::print( std::ostream&, const Universe& ) const
{
	return print( fp );
}

} // namespace barzer 

