#include <barzer_basic_types.h>
#include <barzer_parse_types.h>
#include <barzer_dtaindex.h>
#include <barzer_universe.h>
#include <barzer_storage_types.h>

#include <time.h>

namespace barzer {

uint8_t BarzerDate::thisMonth = 0;
uint8_t BarzerDate::thisDay = 0;
int16_t BarzerDate::thisYear =BarzerDate::INVALID_YEAR;
int32_t BarzerDate::longToday =BarzerDate::INVALID_YEAR;

/// must be called first thing in the program
void BarzerDate::initToday()
{
	time_t t = time(0);
	struct tm theTm = {0};
    localtime_r(&t, & theTm);
	thisMonth = theTm.tm_mon+1;
	thisDay = theTm.tm_mday;
	thisYear = theTm.tm_year;
	longToday = 10000* (int)thisYear + 1000 * (int) thisMonth + thisDay;
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

