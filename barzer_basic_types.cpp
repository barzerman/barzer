#include <barzer_basic_types.h>
#include <barzer_parse_types.h>
#include <barzer_dtaindex.h>
#include <barzer_universe.h>
#include <barzer_storage_types.h>
#include <arch/barzer_arch.h>

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
	time_t t = time(0);
	tm tmdate;
	LOCALTIME_R(&t, &tmdate);
	tmdate.tm_year = year - 1900;
	tmdate.tm_mon = month - 1;
	tmdate.tm_mday = day;
	return mktime(&tmdate);
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

namespace {

static const char* g_BarzerLiteral_typeName[] = {
		"STRING",
		"COMPOUND",
		"STOP",
		"PUNCT",
		"BLANK"
};

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
std::ostream& BarzerLiteral::print( std::ostream& fp, const Universe& ) const
{
	return print( fp );
}

namespace {
struct BarzerRange_print_visitor : public boost::static_visitor<> 
{
	
	std::ostream& fp;

	BarzerRange_print_visitor( std::ostream& f ) : fp(f) {}
	template <typename T> void operator()( const T& t ) const { 
		t.first.print( fp ); 
		fp << ",";
		t.second.print( fp ); 
	}
	void operator () ( const BarzerRange::None& t ) const
	{
		fp << "(null)";
	}
	void operator () ( const BarzerRange::Integer& t ) const
	{
		fp << t.first << "," << t.second ;
	}
	void operator () ( const BarzerRange::Real& t ) const
	{
		fp << t.first << "," << t.second ;
	}
};
} // anonymous namespace ends
std::ostream& BarzerRange::print( std::ostream& fp ) const
{ 
	fp << "Range[";
	BarzerRange_print_visitor visi( fp );
	
	boost::apply_visitor( visi, dta );

	return ( fp << "]" );
}
} // namespace barzer 

