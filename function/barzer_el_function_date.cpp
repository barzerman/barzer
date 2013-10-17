#include <barzer_el_function.h>
#include <barzer_el_function_holder.h>
#include <barzer_datelib.h>
#include <barzer_universe.h>

namespace barzer {
using namespace funcHolder;

namespace {
// applies  BarzerDate/BarzerTimeOfDay/BarzerDateTime to BarzerDateTime
// to construct a timestamp
struct DateTimePacker : public boost::static_visitor<bool> {
    BarzerDateTime &dtim;
    BarzelEvalContext& d_ctxt;
    const char* d_funcName;
    DateTimePacker(BarzerDateTime &d,BarzelEvalContext& ctxt,const char* funcName):
        dtim(d),d_ctxt(ctxt),d_funcName(funcName) {}

    bool operator()(const BarzerDate &data) {
        dtim.setDate(data);
        return true;
    }
    bool operator()(const BarzerTimeOfDay &data) {
        dtim.setTime(data);
        return true;
    }
    bool operator()(const BarzerDateTime &data) {
        if (data.hasDate()) dtim.setDate(data.getDate());
        if (data.hasTime()) dtim.setTime(data.getTime());
        return true;
    }
    bool operator()(const BarzelBeadAtomic &data) {
        return boost::apply_visitor(*this, data.getData());
    }
    // not applicable
    template<class T> bool operator()(const T&)
    {
        pushFuncError(d_ctxt,d_funcName, "Wrong argument type" );
        return false;
    }

};

FUNC_DECL(mkDate) //(d) | (d,m) | (d,m,y) where m can be both number or entity
{
    SETFUNCNAME(mkDate);

    //sets everything to today
    BarzerNumber m(BarzerDate::thisMonth),
                 d(BarzerDate::thisDay),
                 y(BarzerDate::thisYear),
                 tmp;
    BarzerDate &date = setResult(result, BarzerDate());

    bool explicit_Month = false, explicit_Year = false, explicit_Day = false;
    // changes fields to whatever was set
    try {
        switch (rvec.size()) {
        case 3: 
            y = getNumber(rvec[2]);
            explicit_Year = true;
        case 2: { // Do we need to check if ent is ent(1,3) or not ?
            const BarzerEntity* be = getAtomicPtr<BarzerEntity>(rvec[1]);
            m = (be? BarzerNumber(h->gpools.dateLookup.resolveMonthID(q_universe,be->getTokId())) :getNumber(rvec[1]));
            explicit_Month = true;
        }
        case 1: 
            d = getNumber(rvec[0]);
            explicit_Day= true;
        case 0: break; // 0 arguments = today

        default: // size > 3
        FERROR(boost::format("Expected max 3 arguments,%1%  given") % rvec.size());
            y = getNumber(rvec[2]);
            m = getNumber(rvec[1]);
            d = getNumber(rvec[0]);
            break;
        }
        if( bool futureBias = q_universe.checkBit( UBIT_DATE_FUTURE_BIAS ) ) {
            if( explicit_Month ) { // month day were set  
                if( m.getInt() <= BarzerDate::thisMonth ) 
                    y+=1;
            } else if( explicit_Day ) { // only day was set
                if( d.getInt() <= BarzerDate::thisDay )
                    m+=1;
            }
        }
        date.setDayMonthYear(d,m,y);

        return true;
    } catch (boost::bad_get) {
        FERROR( "Wrong argument type"  );

        date.setDayMonthYear(d,m,y);
        return true;
    }
    return false;
}

FUNC_DECL(mkDateRange)
{
    // SETSIG(mkDateRange(Date, Number, [Number, [Number]]));
    SETFUNCNAME(mkDateRange);

    int day = 0, month = 0, year = 0;
    // what is this stuf?!??!  (AY)
    try {
        switch(rvec.size()) {
        case 4: year = getAtomic<BarzerNumber>(rvec[3]).getInt();
        case 3: month = getAtomic<BarzerNumber>(rvec[2]).getInt();
        case 2: {
            day = getAtomic<BarzerNumber>(rvec[1]).getInt();
            const BarzerDate &date = getAtomic<BarzerDate>(rvec[0]);
            BarzerDate_calc c;
            c.setNowPtr ( ctxt.getNowPtr() ) ;
            c.set(date.year + year, date.month + month, date.day + day);
            BarzerRange range;
            if( date < c.d_date )
                range.setData(BarzerRange::Date(date, c.d_date));
            else 
                range.setData(BarzerRange::Date(c.d_date,date));
            setResult(result, range);
            return true;
        }
        default:
                       FERROR( boost::format("Expected 2-4 arguments,%1%  given") % rvec.size() );
        }
    } catch (boost::bad_get) {
                 FERROR( "Wrong argument type");
    }
    return false;
}

FUNC_DECL(mkDay) {
    // SETSIG(mkDay(Number));
    SETFUNCNAME(mkDay);

    if (!rvec.size()) {
        FERROR("Need an argument");
        return false;
    }
    BarzerDate_calc calc;
    calc.setNowPtr ( ctxt.getNowPtr() ) ;
    calc.setToday();
    try {
        calc.dayOffset(getNumber(rvec[0]).getInt());
        setResult(result, calc.d_date);
    } catch(boost::bad_get) {
        FERROR("Wrong argument type");
        setResult(result, calc.d_date);
    }
    return true;
}

FUNC_DECL(mkWday)
{
    SETFUNCNAME(mkWday);

    if (rvec.size() < 2) {
        FERROR("Need 2 arguments");
        return false;
    }
    try {
        int fut  = getAtomic<BarzerNumber>(rvec[0]).getInt();
        uint8_t wday = getAtomic<BarzerNumber>(rvec[1]).getInt();
        BarzerDate_calc calc(fut);
        calc.setNowPtr ( ctxt.getNowPtr() ) ;
        calc.setToday();
        calc.setWeekday(wday);
        setResult(result, calc.d_date);
        return true;
    } catch (boost::bad_get) {
        FERROR("Wrong arg type");
    }
    return false;
}

FUNC_DECL(mkWeekRange)
{
    SETFUNCNAME(mkWeekRange);
    
    try
    {
        int offset = ( rvec.size() ? getAtomic<BarzerNumber>(rvec[0]).getInt() : 0 );
        
    BarzerDate_calc calc;
        if (rvec.size() > 1)
            calc.d_date = getAtomic<BarzerDate>(rvec[1]);
        else
        {
            calc.setNowPtr ( ctxt.getNowPtr() ) ;
            calc.setToday();
        }
        
        std::pair<BarzerDate, BarzerDate> pair;
        calc.getWeek(pair, offset);
        
        BarzerRange range;
        range.dta = pair;
        
        setResult(result, range);
        
        return true;
    }
    catch (const boost::bad_get&)
    {
        FERROR("Wrong arg type");
    }
    catch (const std::exception& e)
    {
        FERROR(e.what());
    }
    
    return false;
}

FUNC_DECL(mkMonthRange)
{
    SETFUNCNAME(mkMonthRange);
    
    try
    {
        int offset = 0;
        if (rvec.size() > 0)
            offset = getAtomic<BarzerNumber>(rvec[0]).getInt();
        
        BarzerDate_calc calc;
        if (rvec.size() > 1)
            calc.d_date = getAtomic<BarzerDate>(rvec[1]);
        else
        {
            calc.setNowPtr ( ctxt.getNowPtr() ) ;
            calc.setToday();
        }
			
        std::pair<BarzerDate, BarzerDate> pair;
        calc.getMonth(pair, offset);
        
        BarzerRange range;
        range.dta = pair;
        
        setResult(result, range);
        
        return true;
    }
    catch (const boost::bad_get&)
    {
        FERROR("Wrong arg type");
    }
    catch (const std::exception& e)
    {
        FERROR(e.what());
    }
    
    return false;
}

FUNC_DECL(mkMonth)
{
    SETFUNCNAME(mkMonth);
    if (rvec.size() < 2) {
        FERROR("Wrong arg type");
        return false;
    }
    try {
        int fut  = getAtomic<BarzerNumber>(rvec[0]).getInt();
        uint8_t month = getAtomic<BarzerNumber>(rvec[1]).getInt();
            BarzerDate_calc calc(fut);
        calc.setNowPtr ( ctxt.getNowPtr() ) ;
        calc.setToday();
        calc.setMonth(month);
        setResult(result, calc.d_date);
        return true;
    } catch (boost::bad_get) {
        FERROR("Wrong argument type");
    }
    return false;
}

FUNC_DECL(mkWdayEnt)
{
    SETFUNCNAME(mkWdayEnt);
    uint wnum(BarzerDate().getWeekday()); // that can be done softer, YANIS
    try {
        if (rvec.size()) {                      
            const BarzerLiteral* bl = getAtomicPtr<BarzerLiteral>(rvec[0]);
            const BarzerString* bs = getAtomicPtr<BarzerString>(rvec[0]);
            const BarzerNumber* n = getAtomicPtr<BarzerNumber>(rvec[0]);
            if (bl) wnum = h->gpools.dateLookup.lookupWeekday(q_universe,*bl);
            else if (bs) wnum = h->gpools.dateLookup.lookupWeekday(q_universe,bs->getStr().c_str());
            else if (n) wnum = ((n->isInt() && n->getInt() > 0 && n->getInt() < 8 )? n->getInt(): 0 );
            else {  
                    FERROR("Wrong argument type");
                    return false;
            }                       
            if (!wnum) {
                    FERROR("Unknown weekday name given");
                    return false;
            }                      
        }    
        uint32_t mid = h->gpools.dateLookup.getWdayID(q_universe,wnum);
        //check if mid exists
        const StoredEntityUniqId euid( mid, 1, 4);//put constants somewhere! c1;s4 == weekdays
        setResult(result, euid);
        return true;
    } catch(boost::bad_get) {
        FERROR("Wrong argument type");
    }
    return false;
}    

FUNC_DECL(mkMonthEnt)
{
    SETFUNCNAME(mkMonthEnt);
    uint mnum(BarzerDate::thisMonth);
    try {
            if (rvec.size()) {                      
                    const BarzerLiteral* bl = getAtomicPtr<BarzerLiteral>(rvec[0]);
                    const BarzerString* bs = getAtomicPtr<BarzerString>(rvec[0]);
                    const BarzerNumber* n = getAtomicPtr<BarzerNumber>(rvec[0]);
                    if (bl) mnum = h->gpools.dateLookup.lookupMonth(q_universe,*bl);
                    else if (bs) mnum = h->gpools.dateLookup.lookupMonth(q_universe,bs->getStr().c_str());
                    else if (n) mnum = ((n->isInt() && n->getInt() > 0 && n->getInt() < 13 )? n->getInt(): 0 );
                    else {  
                            FERROR("Wrong argument type");
                            return false;
                    }                       
                    if (!mnum) {
                            FERROR("Unknown month given");
                            return false;
                    }                      
            }    
        uint32_t mid = h->gpools.dateLookup.getMonthID(q_universe,mnum);
        //check if mid exists
        const StoredEntityUniqId euid( mid, 1, 3);//put constants somewhere! c1;s3 == months
        setResult(result, euid);
            return true;
    } catch(boost::bad_get) {
        FERROR("Wrong argument type");
    }
    return false;
}
FUNC_DECL(mkTime)
{
    SETFUNCNAME(mkTime);
    BarzerTimeOfDay &time = setResult(result, BarzerTimeOfDay());
    try {
        int hh(0), mm(0), ss(0);
        switch (rvec.size()) {
        case 3: {
            const BarzelBeadData &bd = rvec[2].getBeadData();
            if (bd.which()) ss = getNumber(bd).getInt();
        }
        case 2: {
            const BarzelBeadData &bd = rvec[1].getBeadData();
            if (bd.which()) mm = getNumber(bd).getInt();
        }
        case 1: {
        const BarzelBeadData &bd = rvec[0].getBeadData();
            if (bd.which()) hh = getNumber(bd).getInt();
            break;
        }
        case 0: {
            std::time_t t = std::time(0);

            struct tm tmpTm = {0};
            struct tm *tm = localtime_r(&t, &tmpTm);
            hh = tm->tm_hour;
            mm = tm->tm_min;
            ss = tm->tm_sec;
        }
        // note no break here anymore
        default:
            if( rvec.size() > 3 ) {
                FERROR("Expected max 3 arguments");
            }
        }
        time.setHHMMSS( hh, mm, ss );
        return true;
    } catch (boost::bad_get) {
        FERROR("Wrong argument type");
    }
    return false;
}

FUNC_DECL(mkDateTime) {
    SETFUNCNAME(mkDateTime);
    BarzerDateTime dtim;
    DateTimePacker v(dtim,ctxt,func_name);

    for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
            ri != rvec.end(); ++ri) {
        if (!boost::apply_visitor(v, ri->getBeadData())) {
            FERROR("fail");
            return false;
        }
    }
    setResult(result, dtim);
    return true;
}

FUNC_DECL(getWeekday) {
    SETFUNCNAME(getWeekday);
    BarzerDate bd;
    try {
        if (rvec.size())
            bd = getAtomic<BarzerDate>(rvec[0]);
        else bd.setToday();

        BarzerNumber n(bd.getWeekday());
        setResult(result, n);
        return true;
    } catch (boost::bad_get) {
        FERROR("wrong argument type, date expected");
    }
    return false;
}

FUNC_DECL(getMDay) {
    SETFUNCNAME(getMDay);
    BarzerDate bd;
    try {
        if (rvec.size())
            bd = getAtomic<BarzerDate>(rvec[0]);
        else bd.setToday();
        setResult(result, BarzerNumber(bd.day));
        return true;
    } catch (boost::bad_get) {
        FERROR("wrong argument type");
    }
    return false;
}

FUNC_DECL(setMDay) {
    SETFUNCNAME(setMDay);
    BarzerDate bd;
    BarzerNumber n;
    try {
        switch (rvec.size()) {
        case 2: n = getAtomic<BarzerNumber>(rvec[1]);
        case 1: bd = getAtomic<BarzerDate>(rvec[0]); break;
        case 0:
            bd.setToday();
            n.set(1);
        }
        bd.setDay(n);
        setResult(result, bd);
        return true;
    } catch (boost::bad_get) {
        FERROR("wrong argument type");
        setResult(result, bd);
    }
    return true;
}

FUNC_DECL(getMonth) {
    SETFUNCNAME(getMonth);
    BarzerDate bd;
    try {
        if (rvec.size())
            bd = getAtomic<BarzerDate>(rvec[0]);
        else bd.setToday();
        setResult(result, BarzerNumber(bd.month));
        return true;
    } catch (boost::bad_get) {
        FERROR("wrong argument type");
        setResult(result,BarzerNumber(0));
        return true;
    }
}
FUNC_DECL(getYear) {
    SETFUNCNAME(getYear);
    BarzerDate bd;
    try {
        if (rvec.size())
            bd = getAtomic<BarzerDate>(rvec[0]);
        else bd.setToday();
        setResult(result, BarzerNumber(bd.year));
        return true;
    } catch (boost::bad_get) {
        FERROR("wrong argument type");
        setResult(result,BarzerNumber(0));
        return true;
    }
}

FUNC_DECL(getTime)
{
    SETFUNCNAME(getTime);
    if (rvec.size()) {
        const BarzerDateTime* dt = getAtomicPtr<BarzerDateTime>(rvec[0]);
        if (dt) {
            BarzerTimeOfDay t(dt->getTime());
            setResult(result, t);
            return true;
        } else {
            FERROR("Wrong argument type. DateTime expected");
            return false;
        }
    } else { FERROR("At least one argument is needed"); }
    return true;
}

FUNC_DECL(getDate)
{
    SETFUNCNAME(getDate);
    if (rvec.size()) {
        const BarzerDateTime* dt = getAtomicPtr<BarzerDateTime>(rvec[0]);
        if (dt) {
            BarzerDate d(dt->getDate());
            setResult(result, d);
            return true;
        } else {
            FERROR("Wrong argument type. DateTime expected");
            return false;
        }
    } else { 
        BarzerDate_calc calc;
        calc.setNowPtr ( ctxt.getNowPtr() ) ;
        calc.setToday();
        setResult(result,calc.getDate());
    }
    return true;
}

FUNC_DECL(opDateCalc)
{
    SETFUNCNAME(opDateCalc);
    // const char *sig = "opDateCalc(Date ,Number[, Number[, Number]]):";
    try {

        int month = 0, year = 0, day = 0;
        switch (rvec.size()) {
        case 4: year = getAtomic<BarzerNumber>(rvec[3]).getInt();
        case 3: month = getAtomic<BarzerNumber>(rvec[2]).getInt();
        case 2: {
            day = getAtomic<BarzerNumber>(rvec[1]).getInt();
            const BarzerDate &date = getAtomic<BarzerDate>(rvec[0]);
            BarzerDate_calc c;
            c.setNowPtr ( ctxt.getNowPtr() ) ;
            c.set(date.year + year,
                  date.month + month,
                  date.day + day);
            setResult(result, c.d_date);
            return true;
        }
        default:
                FERROR("Need 2-4 arguments" );
        }
    } catch (boost::bad_get) {
        FERROR("Type mismatch");
    }
    return false;
}

/// opTimeCalc(Time,HoursOffset[,MinOffset[,SecOffset]])	
FUNC_DECL(opTimeCalc)   
{
    SETFUNCNAME(opTimeCalc);
    try {
        int h = 0, m = 0, s = 0;
        switch (rvec.size()) {
        case 4:
            s = getAtomic<BarzerNumber>(rvec[3]).getInt();
        case 3:
            m = getAtomic<BarzerNumber>(rvec[2]).getInt();
        case 2: {
            h = getAtomic<BarzerNumber>(rvec[1]).getInt();
            const BarzerTimeOfDay &time = getAtomic<BarzerTimeOfDay>(rvec[0]);
            BarzerTimeOfDay out(time.getHH() + h, time.getMM() + m, time.getSS() + s);
            setResult(result, out);
            return true;
        }
        default:
            FERROR("Need 2-4 arguments" );
        }
    } catch (boost::bad_get) {
        FERROR("Type mismatch");
    }
    return false;
}
    

BELFunctionStorage_holder::DeclInfo g_funcs[] = {

    FUNC_DECLINFO_INIT(opDateCalc, ""),
    FUNC_DECLINFO_INIT(opTimeCalc, ""),

    FUNC_DECLINFO_INIT(getWeekday, ""),      // getWeekday( [BarzerDate] )
    FUNC_DECLINFO_INIT(getTime, ""),         // getTime(DateTime)
    FUNC_DECLINFO_INIT(getDate, ""),         // getDate(DateTime)
    FUNC_DECLINFO_INIT(getMDay, ""),
    FUNC_DECLINFO_INIT(setMDay, ""),
    FUNC_DECLINFO_INIT(getMonth, ""),
    FUNC_DECLINFO_INIT(getYear, ""),

    FUNC_DECLINFO_INIT(mkDate, "makes a date"),
    FUNC_DECLINFO_INIT(mkDateRange, ""),
    FUNC_DECLINFO_INIT(mkDay, ""),
    FUNC_DECLINFO_INIT(mkWday, ""),
    FUNC_DECLINFO_INIT(mkWeekRange, ""),
    FUNC_DECLINFO_INIT(mkMonthRange, ""),
    FUNC_DECLINFO_INIT(mkMonth, ""),
    FUNC_DECLINFO_INIT(mkWdayEnt, ""),
    FUNC_DECLINFO_INIT(mkMonthEnt, ""),
    FUNC_DECLINFO_INIT(mkTime, ""),
    FUNC_DECLINFO_INIT(mkDateTime, "")
};

} // anonymous namespace

namespace funcHolder {
void loadAllFunc_date(BELFunctionStorage_holder* holder)
    { for( const auto& i : g_funcs ) holder->addFun( i ); }
}

} // namespace barzer
