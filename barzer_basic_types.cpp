/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_basic_types.h>
#include <barzer_parse_types.h>
#include <barzer_dtaindex.h>
#include <barzer_universe.h>
#include <barzer_storage_types.h>
#include <arch/barzer_arch.h>
//#include <barzer_chain.h>

#include <time.h>
#include <arch/barzer_arch.h>
namespace barzer {

//// BarzerNumber 


//// end of BarzerNumber methonds
uint8_t BarzerDate::thisMonth = 0;
uint8_t BarzerDate::thisDay = 0;
int16_t BarzerDate::thisYear =BarzerDate::INVALID_YEAR;
int32_t BarzerDate::longToday =BarzerDate::INVALID_YEAR;

/// must be called first thing in the program
void BarzerDate::initToday()
{
	time_t t = time(0);
	struct tm theTm = {0};
    LOCALTIME_R(&t, & theTm);
	thisMonth = theTm.tm_mon+1;
	thisDay = theTm.tm_mday;
	thisYear = theTm.tm_year + 1900;
	longToday = 10000* (int)thisYear + 1000 * (int) thisMonth + thisDay;
}

uint8_t BarzerDate::getWeekday() const
{
	time_t t = time(0);
	tm tmdate;
	LOCALTIME_R(&t, &tmdate);
	tmdate.tm_year = year - 1900;
	tmdate.tm_mon = month - 1;
	tmdate.tm_mday = day;
	mktime(&tmdate);
	return (tmdate.tm_wday + 6) % 7 + 1;
}

time_t BarzerDate::getTime_t() const
{
	tm tmdate={0};
	tmdate.tm_year = year - 1900;
	tmdate.tm_mon = month - 1;
	tmdate.tm_mday = day;
	return mktime(&tmdate);
}


	void BarzerDate::setDayMonthYear(const BarzerNumber& d, const BarzerNumber& m, const BarzerNumber& y ) 
    {
		day = (uint8_t) d.getInt();
		month = (uint8_t) m.getInt();
		year = (int16_t) y.getInt();

        tm tmdate={0};
	    time_t t = time(0);
        tmdate.tm_year = year - 1900;
        tmdate.tm_mon = month - 1;
        tmdate.tm_mday = day;

        mktime(&tmdate);
        
        day = tmdate.tm_mday;
        month = tmdate.tm_mon + 1;
        year = tmdate.tm_year + 1900;
	}
void BarzerDate::setTime_t(time_t t)
{
	struct tm tmdate;
	LOCALTIME_R(&t, &tmdate);
	year = tmdate.tm_year + 1900;
	month = tmdate.tm_mon + 1;
	day = tmdate.tm_mday;
}


void BarzerDate::setToday() {
	day = thisDay;
	month = thisMonth;
	year = thisYear;
}


/// will try to set year to current year 
void BarzerDate::setDayMonth(const BarzerNumber& d, const BarzerNumber& m) 
{
	month = m.getInt();
	day = d.getInt();
	year = thisYear;
}

	void BarzerDate::setYYYYMMDD( int x )
	{
		year  = x/10000;
		month = (x%10000)/100;
		day = x%100;
	}
	std::ostream& BarzerDate::print( std::ostream& fp ) const
		{ return ( fp << std::dec << (int)month << '/' << (int)day << '/' << year ); }
    
    std::istream& BarzerDate::deserialize( std::istream& fp ) 
    {
        char c;
        int y, m, d;
        fp >> y >> c >> m>> c >> d ;
        year = y;
        month = m;
        day = d;
         
        return fp;
    }
	bool BarzerDate::lessThan( const BarzerDate& r ) const
	{
		return ay::range_comp( ).less_than(
			year,	month, day,
			r.year,	r.month, r.day
		);
	}
	bool BarzerDate::isEqual( const BarzerDate& r ) const
	    { return( (year == r.year) && (month == r.month) && (day  == r.day )); }
namespace {

static const char* g_BarzerLiteral_typeName[] = {
		"STRING",
		"COMPOUND",
		"STOP",
		"PUNCT",
		"BLANK"
};

}

std::pair< const char*, size_t> BarzerLiteral::toString (const StoredUniverse& universe ) const
{
    switch(getType()) {
    case BarzerLiteral::T_STRING:
    case BarzerLiteral::T_COMPOUND:
    case BarzerLiteral::T_STOP:
        {
        const char* s = universe.getStringPool().resolveId(theId);
        return std::pair< const char*, size_t>( s, (s? strlen(s): 0 ) );
        }
    case BarzerLiteral::T_PUNCT:
		return std::pair< const char*, size_t>( (const char*)( &theId ), 1 );
    case BarzerLiteral::T_BLANK:
		return std::pair< const char*, size_t>( " ", 1 ) ;
    }
    return std::pair< const char*, size_t>( 0, 0 );
}

bool BarzerLiteral::toBNumber (BarzerNumber& n ) const
{
	switch (getNumeralType ())
	{
		case NUMERAL_TYPE_INT:
            n.set( d_num.i4);
			return true;
		case NUMERAL_TYPE_REAL:
            n.set( d_num.r4);
			return true;
		default:
			return false;
	}
}

const char* BarzerLiteral::getTypeName(int t) 
{
	return( ARR_NONNULL_STR( g_BarzerLiteral_typeName, (unsigned)t) ) ;
}

std::ostream& BarzerLiteral::print( std::ostream& fp ) const
{
	return ( fp << getTypeName(type) << ":" << std::hex << theId );
}

std::ostream& BarzerLiteral::print( std::ostream& fp, const BELPrintContext& ) const
{
	return print( fp );
}

} // namespace barzer 

