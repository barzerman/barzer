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

} // namespace barzer 

