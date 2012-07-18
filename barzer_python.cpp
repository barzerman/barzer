#include <barzer_universe.h>
#include <barzer_shell.h>
#include <boost/python.hpp>
#include <barzer_python.h>
#include <ay/ay_cmdproc.h>
#include <ay_translit_ru.h>
#include <boost/python/list.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>

#include <barzer_parse.h>
#include <barzer_server_response.h>
#include <barzer_el_chain.h>
#include <barzer_emitter.h>
#include <boost/variant.hpp>

#include <util/pybarzer.h>

#include <autotester/barzer_at_comparators.h>


using boost::python::stl_input_iterator ;
using namespace boost::python;
using namespace boost;

namespace barzer {

struct QueryParseEnv {
    QParser qparse;
    Barz barz;
    QuestionParm qparm;

    QueryParseEnv( const StoredUniverse& u ) :
        qparse(u)
    {}

    void parseXML( BarzStreamerXML& streamer, std::ostream& fp, const char* q ) 
    {
        qparse.parse( barz, q, qparm );
        streamer.print( fp );
    }
};

PythonCmdLine::~PythonCmdLine() 
    { delete d_cmdlProc; }
PythonCmdLine::PythonCmdLine():d_cmdlProc(new ay::CommandLineArgs)
    {} 
PythonCmdLine&  PythonCmdLine::init( boost_python_list& ns )
{
    // stl_input_iterator<boost_python_list> ii(ns);
    ns[0];
    d_argv_str.clear();
    size_t ns_len = len(ns);
    for (size_t i = 0; i < ns_len; ++i) {
        d_argv_str.push_back( extract<std::string>(ns[i]) );
    }
    argv.clear();
    argv.reserve( d_argv_str.size() );
    for( std::vector< std::string >::const_iterator i = d_argv_str.begin(); i!= d_argv_str.end(); ++i ) {
        argv.push_back( i->c_str() );
    }
    /// dirty hack - ay cmdline processor wants char** ...
    d_cmdlProc->init(argv.size(), (char**)(&(argv[0])) );
    
    return *this;
}

BarzerPython::BarzerPython() : 
    gp(new GlobalPools),
    shell(0),
    d_universe(0),
    d_parseEnv(0)
{}

BarzerPython::~BarzerPython()
{
    delete shell;
    delete gp;
    delete d_parseEnv;
}

int BarzerPython::init( boost_python_list& ns )
{
    gp->init_cmdline( d_pythCmd.init(ns).cmdlProc() );
    d_universe = gp->getUniverse(0);

    d_parseEnv = new QueryParseEnv( *d_universe );
    return 0;
}

std::string BarzerPython::translitEn2Ru(const std::string& from/*, const ay::tl::TLExceptionList_t& excs*/)
{
	std::string result;
	ay::tl::en2ru(from.c_str(), from.size(), result);
	return result;
}

std::string BarzerPython::translitRu2En(const std::string& from/*, const ay::tl::TLExceptionList_t& excs*/)
{
	std::string result;
	ay::tl::ru2en(from.c_str(), from.size(), result);
	return result;
}

void BarzerPython::shell_cmd( const std::string& cmd, const std::string& args )
{
    if( !shell ) 
        shell = new BarzerShell(0,*gp);
}

boost_python_list BarzerPython::guessLang(const std::string& s)
{
	list l;
	if (!d_universe)
		return l;

	auto& pools = d_universe->getGlobalPools();

	std::vector<std::pair<int, double>> probs;
	ay::evalAllLangs(pools.getUTF8LangMgr(), pools.getASCIILangMgr(), s.c_str(), probs);

	for (size_t i = 0; i < probs.size(); ++i)
	{
		const char *lang = ay::StemWrapper::getValidLangString(probs[i].first);
		list pair;
		pair.append(lang ? lang : "unknown lang");
		pair.append(probs.at(i).second);
		l.append(pair);
	}

	return l;
}

int BarzerPython::matchXML(const std::string& pattern, const std::string& result, const autotester::CompareSettings& settings)
{
	if (!d_universe)
		return -1;

	return autotester::matches(pattern.c_str(), result.c_str(),
			{ d_universe->getGlobalPools(), d_universe->getUserId() },
			settings);
}

std::string BarzerPython::bzstem(const std::string& s)
{
    if( d_universe ) {
        std::string stem;
        if( d_universe->getBZSpell()->getStemCorrection( stem, s.c_str() ) )
            return stem;
        else 
            return s;
    } else {
        std::cerr << "No Universe set\n";
        const char message[] = "No Universe set"; 
        PyErr_SetString(PyExc_ValueError, message); 
        throw error_already_set(); 
    }
    return s;
}

#define BPY_ERR(s) PyErr_SetString(PyExc_ValueError, #s ); throw error_already_set();
int BarzerPython::setUniverse( const std::string& us )
{
    int uniNum= atoi( us.c_str() ) ;

    StoredUniverse * newUniverse = gp->getUniverse(uniNum);

    if( newUniverse == d_universe ) 
        return uniNum;

    d_universe = newUniverse;

    if( d_universe ) {
        delete d_parseEnv ;
        d_parseEnv = new QueryParseEnv( *d_universe );
        return uniNum;
    } else {
        const char message[] = "Universe not found"; 
        PyErr_SetString(PyExc_ValueError, message); 
        throw error_already_set(); 
    }
}

std::string BarzerPython::parse( const std::string& q )
{
    if ( d_universe ) {
        if ( d_parseEnv ) {
            std::stringstream sstr;
            BarzStreamerXML xmlStreamer( d_parseEnv->barz, *d_universe );
            d_parseEnv->parseXML( xmlStreamer, sstr, q.c_str() );
            return sstr.str();
        } else {
            BPY_ERR("Parse Environment not set")
        }
    } else {
        BPY_ERR("No Universe set")
    }
}

std::string BarzerPython::emit(const std::string& q )
{   
    GlobalPools gp_;
    BELTrie* trie  = gp_.mkNewTrie();

    std::stringbuf  buf;
    std::ostream os(&buf);

    BELReaderXMLEmit reader(trie, os);
    reader.initParser(BELReader::INPUT_FMT_XML);
    reader.setSilentMode();     //what does it for? (copied from barzer_server.cpp:288)
    std::istringstream is(q);
    reader.loadFromStream( is );
    delete trie;
    return buf.str();

}



//// encoding visitors and service functions 
namespace {


//// exposed namespace contains "parallel" implementations of classes which otherwise cant be exported 
/// for example string ids cant be translated into strings without some barzer data (universe) 
/// so, for those classes which have this type of information (for now i cant think of anything 
/// other than string / token ids that woiuld require universe or global pools data for decoding)
/// we will have exposed::XXXX counterparty 
namespace exposed {

struct nv_streamer {
    std::ostream& fp;
    bool hasStuff;
    nv_streamer(std::ostream& o, const char* objName ) : 
        fp(o),hasStuff(false) { fp<< objName << "(";}

    template <typename T>
    nv_streamer& operator()( bool doAnything, const char* name, const T& t )  
    {
        if( doAnything ) {
            if( hasStuff ) fp << ","; else hasStuff = true;
            fp << name << '=' << t;
        }
        return *this;
    }
    nv_streamer& operator()( bool doAnything, const char* name, const uint8_t& t )  
    {
        if( doAnything ) {
            if( hasStuff ) fp << ","; else hasStuff = true;
            fp << name << '=' << (int)t;
        }
        return *this;
    }

    ~nv_streamer() { fp << ")"; }
};

struct BarzerLiteral {
    barzer::BarzerLiteral ltrl;
    std::string txt;
    
    BarzerLiteral( const barzer::BarzerLiteral& l, const StoredUniverse& u ) :
        ltrl(l), txt(u.printableStringById(l.getId()))
    {}
};
///                                                             DATE
struct BarzerDate : public barzer::BarzerDate {
    BarzerDate( const barzer::BarzerDate& d, const StoredUniverse& u ) : 
        barzer::BarzerDate(d) {}
    
    std::ostream& print( std::ostream& fp ) const 
        { return nv_streamer(fp,"date") (isValidDay(),"day",day) (isValidMonth(),"month",month ) (isValidYear(),"year",year ).fp; }
};
// std::ostream& operator<< ( std::ostream& fp, const BarzerDate& d ) { return d.print(fp); }
////                                                            TIME OF DAY 
struct BarzerTimeOfDay : public barzer::BarzerTimeOfDay {

    BarzerTimeOfDay( const barzer::BarzerTimeOfDay& d, const StoredUniverse& u ) : barzer::BarzerTimeOfDay(d) {}
    
    std::ostream& print( std::ostream& fp ) const 
        { return ( isValid() ?  nv_streamer(fp,"time") (true,"hh",getHH()) (true,"mm",getMM()) (true,"ss",getSS() ).fp: fp); }
};
std::ostream& operator<< ( std::ostream& fp, const BarzerTimeOfDay& d ) { return d.print(fp); }
////                                                            (TIMESTAMP) DATE TIME
struct BarzerDateTime {
    barzer::BarzerDateTime barzerTimestamp;

    BarzerTimeOfDay timeOfDay;
    BarzerDate date;
    const StoredUniverse& universe;

    BarzerDateTime( const barzer::BarzerDateTime& d, const StoredUniverse& u ) : 
        barzerTimestamp(d),
        timeOfDay(d.getTime(),u),
        date(d.getDate(),u),
        universe(u) 
    {}
    
    const BarzerDate getDate() const { return date; }
    const BarzerTimeOfDay getTime() const { return timeOfDay; }

    std::ostream& print( std::ostream& fp ) const 
        { return nv_streamer(fp,"timestamp") (barzerTimestamp.hasDate(),"date",date) (barzerTimestamp.hasTime(),"time",timeOfDay).fp; }
};
std::ostream& operator<< ( std::ostream& fp, const BarzerDateTime& d ) { return d.print(fp); }

struct BarzerEntity {
    barzer::BarzerEntity euid;
    uint32_t eclass, esubclass; 
    std::string id;

    bool isValid() const { return euid.isValid(); }
    BarzerEntity() : eclass(euid.eclass.ec),esubclass(euid.eclass.subclass) {}
    BarzerEntity( const barzer::BarzerEntity& e, const StoredUniverse& u ) :
        euid(e),
        eclass      (euid.getClass().ec), 
        esubclass   (euid.getClass().subclass)
    {
        const char *tokname = u.getGlobalPools().internalString_resolve(euid.tokId);

        if( tokname )
            id.assign(tokname);
    }
    std::ostream& print( std::ostream& fp ) const {
        return( fp << "ent(cl=" << eclass << ",scl=" << esubclass << ",id='" << id << "'" << ")" );
    }
    static std::string str(BarzerEntity const &self) {
         std::stringstream sstr;
         self.print(sstr);
         return sstr.str();
    }
};
std::ostream& operator<< ( std::ostream& fp, const BarzerEntity& ent ) { return ent.print(fp); }

struct TopicInfo {
    BarzerEntity ent;
    int weight;
    TopicInfo() : weight(0) {}
    TopicInfo( const barzer::BarzerEntity& e, int w, const StoredUniverse& u ) :
        ent(e,u), weight(w) {}
    std::ostream& print( std::ostream& fp ) const 
    {
        return ( fp << "(" << ent << ",w=" << weight << ")" );
    }

};
std::ostream& operator<<( std::ostream& fp, const TopicInfo& t )
    { return t.print(fp); }

struct BarzerRange {
    barzer::BarzerRange br;

    list rng;
    
    BarzerRange( const barzer::BarzerRange& r, const StoredUniverse& u );
    
    object lo() { return rng[0]; } 
    object hi() { return rng[1]; } 

    bool hasHi() const { return br.hasHi(); }
    bool hasLo() const { return br.hasLo(); }

	bool isNone( ) const { return br.isNone(); }
	bool isValid( ) const {return br.isValid(); }
	bool isInteger( ) const {return br.isInteger(); }
	bool isReal( ) const {return br.isReal(); }
    bool isNumeric() const { return br.isNumeric(); }
	bool isTimeOfDay( ) const {return br.isTimeOfDay(); }
	bool isDate( ) const {return br.isDate(); }
	bool isDateTime( ) const {return br.isDateTime(); }
	bool isEntity( ) const {return br.isEntity(); }

    bool isAsc() const { return br.isAsc(); }
    bool isDesc() const { return br.isDesc(); }
    
    std::ostream& printBoundary( std::ostream& fp, bool isLo ) const;
    std::ostream& print( std::ostream& fp ) const ;
};


struct BarzerRange_printer_visitor : public static_visitor<void> {
    const BarzerRange& br;
    std::ostream& fp;
    int lohi; // 0 - lo, 1 - hi

    BarzerRange_printer_visitor( const BarzerRange& r, std::ostream& os, bool isLo ) : 
        br(r), fp(os), lohi(isLo? 0:1) {}


    void operator()( const barzer::BarzerRange::Real& r ) 
        { fp << extract<double>(br.rng[lohi]); }
    void operator()( const barzer::BarzerRange::Integer& r ) 
        { fp << extract<int64_t>(br.rng[lohi]); }
    void operator()( const barzer::BarzerRange::Entity& r ) 
        { fp << extract<BarzerEntity>(br.rng[lohi]); }
    void operator()( const barzer::BarzerRange::Date& r ) 
        { 
            const BarzerDate& bd = extract<BarzerDate>(br.rng[lohi]);
            fp << bd;
        }
    void operator()( const barzer::BarzerRange::DateTime& r ) 
        { fp << extract<BarzerDateTime>(br.rng[lohi]); }
    void operator()( const barzer::BarzerRange::TimeOfDay& r ) 
        { 
            const BarzerTimeOfDay& tod = extract<BarzerTimeOfDay>(br.rng[lohi]); 
            fp << tod;
        }
    template <typename T>
    void operator()( const T& r ) 
    {
        fp << "range(type=" << br.br.dta.which() << ")" ;
    }
    
};

std::ostream& BarzerRange::printBoundary( std::ostream& fp, bool isLo ) const 
{
    BarzerRange_printer_visitor v(*this,fp,isLo);
    apply_visitor( v, br.dta );
    return fp;
}

std::ostream& BarzerRange::print( std::ostream& fp ) const 
{
    fp << "range(";
    if( hasLo() ) {
        fp << "lo=";
        printBoundary(fp,true);
    }
    if( hasHi() ) {
        if( hasLo() ) fp << ",";
        fp << "hi=";
        printBoundary(fp,false);
    }
    return fp << ")";
}
std::ostream& operator<<( std::ostream& fp, const BarzerRange& r ) 
    { return r.print(fp); }

struct BarzerRange_packer_visitor : public static_visitor<void> {
    BarzerRange& b;
    const StoredUniverse& universe;

    BarzerRange_packer_visitor( BarzerRange& bb, const StoredUniverse& u ) : b(bb), universe(u) {}

    void operator()( const barzer::BarzerRange::Entity& i )
    {
        b.rng.append( BarzerEntity(i.first,universe) );
        b.rng.append( BarzerEntity(i.second,universe) );
    }
    void operator()( const barzer::BarzerRange::DateTime& i )
    {
        b.rng.append( BarzerDateTime(i.first,universe) );
        b.rng.append( BarzerDateTime(i.second,universe) );
    }
    void operator()( const barzer::BarzerRange::TimeOfDay& i )
    {
        b.rng.append( BarzerTimeOfDay(i.first,universe) );
        b.rng.append( BarzerTimeOfDay(i.second,universe) );
    }
    void operator()( const barzer::BarzerRange::Date& i )
    {
        b.rng.append( BarzerDate(i.first,universe) );
        b.rng.append( BarzerDate(i.second,universe) );
    }
    template <typename T>
    void operator()( const T& i )
    {
        b.rng.append( i.first );
        b.rng.append( i.second );
    }
};

inline BarzerRange::BarzerRange( const barzer::BarzerRange& r, const StoredUniverse& u ) :
    br(r)
{ 
    BarzerRange_packer_visitor v(*this,u);
    apply_visitor( v, br.getData() ); 
}


struct BarzerEntityRangeCombo {
    BarzerEntity mainEntity;
    BarzerEntity unitEntity;
    BarzerRange  range;

    BarzerEntityRangeCombo( const barzer::BarzerEntityRangeCombo& erc, const StoredUniverse& u ) :
        mainEntity(erc.mainEntity(),u),
        unitEntity(erc.unitEntity(),u),
        range(erc.range(),u)
    {}
    std::ostream&  print( std::ostream& fp) const 
    {
        return nv_streamer(fp,"erc")   
            (mainEntity.isValid(),"ent",mainEntity) 
            (unitEntity.isValid(),"unit",unitEntity) 
            (true,"range",range)
            .fp;
    }
};
std::ostream& operator<<( std::ostream& fp, const BarzerEntityRangeCombo& erc ) { return erc.print(fp); }

struct TraceInfo {
    const barzer::BarzelTrace::SingleFrameTrace frameTrace;
    std::string sourceStr;
    list        err;
    TraceInfo( const barzer::BarzelTrace::SingleFrameTrace& ft, const StoredUniverse& u ) :
        frameTrace(ft) {
            const char* tmp = u.getGlobalPools().internalString_resolve( ft.tranInfo.source );
            if( tmp ) sourceStr.assign(tmp);
            for( std::vector< std::string >::const_iterator ei = ft.errVec.begin(); ei!= ft.errVec.end(); ++ei ) {
                err.append( *ei );
            }
        }

    std::ostream& print( std::ostream& fp ) const 
    {
        fp << "frame( src='" << sourceStr << "'-" << frameTrace.tranInfo.statementNum << ":" << frameTrace.tranInfo.emitterSeqNo <<
        ",gr="<< frameTrace.grammarSeqNo ;
         
        size_t err_len = len(err);
        for (size_t i = 0; i < err_len; ++i) {
            std::string s=extract<std::string>(err[i]) ;
            fp << "err(" << s  << ")\n";
        }
        return fp << ")";
    }

    uint32_t emit() const { return frameTrace.tranInfo.emitterSeqNo; }
    uint32_t statement() const { return frameTrace.tranInfo.statementNum; }
    uint32_t grammar() const { return frameTrace.grammarSeqNo; }
    const std::string source() const { return sourceStr; }
};
std::ostream& operator<<( std::ostream& fp, const TraceInfo& ti ) { return ti.print(fp); }

} // exposed namespace 

namespace visitor {
/// this static visitor is packing bead list
struct bead_list : public static_visitor<bool> {
    const StoredUniverse&   d_universe;
    const BarzelBead&       d_bead; 
    list&                   d_list;
    size_t                  d_beadNum;
     
    bead_list( list& l, const StoredUniverse& u, const BarzelBead& b, size_t beadNum ) :
        d_universe(u), d_bead(b), d_list(l), d_beadNum(beadNum)
    {}
     
    bool operator()( const BarzerDateTime& r ) 
        { return (d_list.append(exposed::BarzerDateTime(r,d_universe)),true); }
    bool operator()( const BarzerTimeOfDay& r ) 
        { return (d_list.append(exposed::BarzerTimeOfDay(r,d_universe)),true); }
    bool operator()( const BarzerDate& r ) 
        { return (d_list.append(exposed::BarzerDate(r,d_universe)),true); }
    bool operator()( const BarzerEntityRangeCombo& r ) 
        { return (d_list.append( exposed::BarzerEntityRangeCombo(r,d_universe) ),true); }
    bool operator()( const BarzerRange& r )
        { return ( d_list.append(exposed::BarzerRange(r,d_universe)), true ); }
    bool operator()( const BarzerEntityList& l )
    {
        list elist;
		const BarzerEntityList::EList &lst = l.getList();
		for (BarzerEntityList::EList::const_iterator li = lst.begin();
													 li != lst.end(); ++li) {
			elist.append(
                exposed::BarzerEntity(*li,d_universe)
            );
		}
        return ( d_list.append(elist), true );
    }

    bool operator()( const BarzerEntity& l )
        { return( d_list.append(exposed::BarzerEntity(l,d_universe)), true ); }
    bool operator()( const BarzerNumber& l )
        { 
            if( l.isInt() ) 
                return (d_list.append( l.getInt() ),true); 
            else
                return (d_list.append( l.getReal() ),true); 
        }
    bool operator()( const BarzerString& l )
        { return (d_list.append( l.getStr() ),true); }

    bool operator()( const BarzerLiteral& l )
    {
        if( l.isString() ) 
            d_list.append( std::string(d_universe.printableStringById(l.getId())) );
        else if( l.isPunct() ) {
            std::string ps;
            ps.push_back( (char)( l.getId() ) );
            d_list.append( ps );
        }
        else 
            d_list.append(exposed::BarzerLiteral(l,d_universe));
        
        return true;
    }
    bool operator()( const BarzelBeadAtomic& ba )
        { return( apply_visitor( (*this), ba.getData() ), true ); }

    template <typename T>
    bool operator()( const T& t ) {
        d_list.append(t);
        return false;
    }

    /// 
    void terminateBead() {}
}; /// bead_list encoder 

} // visitor namespace ends

void encodeTraceInfo(list& traceInfoList, const Barz &barz, const StoredUniverse &uni)
{
    const BarzelTrace::TraceVec &tvec = barz.barzelTrace.getTraceVec();
    if( tvec.size() ) {
        for( BarzelTrace::TraceVec::const_iterator ti = tvec.begin(),
                                                  tend = tvec.end();
                    ti != tend; ++ti ) {
            traceInfoList.append( 
                exposed::TraceInfo( *ti , uni )
            );
        }
    }
}

} //  anonymous namespace ends

/// 
struct BarzerResponseObject {
    list beadList;  /// list of beads  

    list topicInfo; /// information on the topics discovered during parse - the list will contain exposed::TopicInfo

    list traceInfo;
    list spellInfo;

    dict misc; 
    
    void init( const StoredUniverse& universe, const Barz& barz ) {
        const BarzelBeadChain &bc = barz.getBeads();
        CToken::SpellCorrections spellCorrections;
        size_t curBeadNum = 1;
        for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
            if (!(bli->isBlank()|| bli->isBlankLiteral()) ) {
                visitor::bead_list v(beadList, universe, *bli, curBeadNum);
                if (apply_visitor(v, bli->getBeadData())) 
                    v.terminateBead();
                ++curBeadNum;
            }


            //// accumulating spell corrections in spellCorrections vector 
            const CTWPVec& ctoks = bli->getCTokens();
            for( CTWPVec::const_iterator ci = ctoks.begin(); ci != ctoks.end(); ++ci ) {
                const CToken::SpellCorrections& corr = ci->first.getSpellCorrections(); 
                spellCorrections.insert( spellCorrections.end(), corr.begin(), corr.end() );
            }
            /// end of spell corrections accumulation
        }
        /// printing topics 
        const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
        if( !topicMap.empty() ) {
            // visitor::bead_list v(topicInfo, universe, fakeBead, curTopicNum );
            for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
                topicInfo.append( exposed::TopicInfo( topI->first, topI->second, universe ) );
            }
        }


        /// printing spell corrections  if any 
        if( spellCorrections.size( ) ) {
            for( CToken::SpellCorrections::const_iterator i = spellCorrections.begin(); i!= spellCorrections.end(); ++i ) {
                 list corr;
                 corr.append(i->first);
                 corr.append(i->second);
                 spellInfo.append( corr );
            }
        }

        encodeTraceInfo(traceInfo, barz, universe);
    }
};

struct PythonQueryProcessor {
    const BarzerPython& bpy;
    Barz d_barz;
    GlobalPools*    local_gp;
    BELTrie*        local_trie;
    
    PythonQueryProcessor(const BarzerPython&b): 
        bpy(b), local_gp(new GlobalPools()), local_trie(local_gp->mkNewTrie())
    {}
    ~PythonQueryProcessor()
        { delete local_gp; delete local_trie; }
    
    const StoredUniverse*         barzeIt( int userNumber, const std::string& q )
    {
        QuestionParm qparm;
        d_barz.clearWithTraceAndTopics();

        const StoredUniverse* universe = bpy.getGP().getUniverse(userNumber);

        if( !universe ) {
            std::stringstream errstr;
            errstr << "user " << userNumber << " doesnt exist";
            PyErr_SetString(PyExc_ValueError, errstr.str().c_str() ); 
            throw error_already_set();
        }

        QParser(*universe).parse( d_barz, q.c_str(), qparm );
        return universe;
    }

    BarzerResponseObject* parse( int userNumber, const std::string& q )
    {
        const StoredUniverse* universe = barzeIt(userNumber, q );
        if( universe ) {
            BarzerResponseObject* resp = new BarzerResponseObject();

            resp->init( *universe, d_barz );
            return resp;
        }

        return 0;
    }
    
    size_t count_emit(const std::string& q ) const
    {
        std::stringbuf buf;
        std::ostream os(&buf);

        BELReaderXMLEmitCounter reader(local_trie, os);   //trie needs to be cut out!
        reader.initParser(BELReader::INPUT_FMT_XML);
        reader.setSilentMode();
        std::istringstream is(q);
        reader.loadFromStream( is );
        return reader.getCounter();
    }    

    std::string parseXML( int userNumber, const std::string& q ) 
    {
        const StoredUniverse* universe = barzeIt(userNumber, q );
        if( universe ) {
            std::stringstream sstr;
            BarzStreamerXML xmlStreamer( d_barz, *universe );
            xmlStreamer.print( sstr );
            return sstr.str();
        }

        return std::string("");
    }
};

PythonQueryProcessor* BarzerPython::makeParseEnv( ) const
{
    return new PythonQueryProcessor(*this);
}


} // barzer

namespace
{
	template<typename Class, typename Val, Class& (Class::*method) (Val)>
	void nonReturn(Class& self, Val val)
	{
		(self.*method)(val);
	}
}

BOOST_PYTHON_MODULE(pybarzer)
{
    AYLOGINIT(DEBUG);

    // AYLOG(DEBUG) << "LOADING BARZER PYTHON MODULE\n";
    boost::python::class_<barzer::BarzerPython>( "Barzer" )
        .def( "stem", &barzer::BarzerPython::bzstem )
        .def( "guessLang", &barzer::BarzerPython::guessLang )
        .def( "init", &barzer::BarzerPython::init )
        .def( "universe", &barzer::BarzerPython::setUniverse )
        .def( "mkProcessor", &barzer::BarzerPython::makeParseEnv, return_value_policy<manage_new_object>() )
        /// returns Barzer XML for the parsed query 
        .def( "parsexml", &barzer::BarzerPython::parse  )
        .def( "emit", &barzer::BarzerPython::emit)
		.def( "ru2en", &barzer::BarzerPython::translitRu2En)
		.def( "en2ru", &barzer::BarzerPython::translitEn2Ru)
        .def( "matchXML", &barzer::BarzerPython::matchXML);

    def("stripDiacritics", stripDiacritics);
    // BarzerResponseObject    
    boost::python::class_<barzer::BarzerResponseObject>( "response" )
        .def_readonly( "trace", &barzer::BarzerResponseObject::traceInfo )
        .def_readonly( "spell", &barzer::BarzerResponseObject::spellInfo )
        .def_readonly( "spell", &barzer::BarzerResponseObject::spellInfo )
        .def_readonly( "topics", &barzer::BarzerResponseObject::topicInfo )
        .def_readwrite( "beads", &barzer::BarzerResponseObject::beadList );

    // boost::python::class_<barzer::BarzerPython>( "Entity" )
    boost::python::class_<barzer::PythonQueryProcessor>( "processor", no_init )
        .def( "parse", &barzer::PythonQueryProcessor::parse, return_value_policy<manage_new_object>() )
        .def( "parsexml", &barzer::PythonQueryProcessor::parseXML )
        .def( "count_emit", &barzer::PythonQueryProcessor::count_emit);

    boost::python::class_<barzer::exposed::BarzerDate>( "Date", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def_readonly( "month", &barzer::exposed::BarzerDate::month )
        .def_readonly( "day", &barzer::exposed::BarzerDate::day )
        .def_readonly( "year", &barzer::exposed::BarzerDate::year )
        ;
    boost::python::class_<barzer::exposed::BarzerTimeOfDay>( "Time", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def( "hh", &barzer::exposed::BarzerTimeOfDay::getHH )
        .def( "mm", &barzer::exposed::BarzerTimeOfDay::getMM )
        .def( "ss", &barzer::exposed::BarzerTimeOfDay::getSS )
        ;
    boost::python::class_<barzer::exposed::BarzerDateTime>( "Timestamp", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def_readwrite( "time", &barzer::exposed::BarzerDateTime::timeOfDay)
        .def_readwrite( "date", &barzer::exposed::BarzerDateTime::date )
        ;

    boost::python::class_<barzer::exposed::BarzerLiteral>( "Token", no_init )
        .def_readonly( "txt", &barzer::exposed::BarzerLiteral::txt );

    boost::python::class_<barzer::exposed::BarzerEntity>( "Entity", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def_readonly( "id", &barzer::exposed::BarzerEntity::id )
        .def_readonly( "cl", &barzer::exposed::BarzerEntity::eclass )
        .def_readonly( "scl", &barzer::exposed::BarzerEntity::esubclass )
        ;
    boost::python::class_<barzer::exposed::TopicInfo>( "Topic" )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def_readwrite( "ent", &barzer::exposed::TopicInfo::ent )
        .def_readwrite( "w", &barzer::exposed::TopicInfo::weight );

    boost::python::class_<barzer::exposed::BarzerRange>( "Range", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def( "lo", &barzer::exposed::BarzerRange::lo )
        .def( "hi", &barzer::exposed::BarzerRange::hi )
        .def( "hasHi", &barzer::exposed::BarzerRange::hasHi )
        .def( "hasLo", &barzer::exposed::BarzerRange::hasLo )

	    .def( "isNone", &barzer::exposed::BarzerRange::isNone )
	    .def( "isValid", &barzer::exposed::BarzerRange::isValid )
	    .def( "isInteger", &barzer::exposed::BarzerRange::isInteger )
	    .def( "isReal", &barzer::exposed::BarzerRange::isReal )
        .def( "isNumeric", &barzer::exposed::BarzerRange::isNumeric )
	    .def( "isTimeOfDay", &barzer::exposed::BarzerRange::isTimeOfDay )
	    .def( "isDate", &barzer::exposed::BarzerRange::isDate )
	    .def( "isDateTime", &barzer::exposed::BarzerRange::isDateTime )
	    .def( "isEntity", &barzer::exposed::BarzerRange::isEntity )

	    .def( "isAsc", &barzer::exposed::BarzerRange::isAsc )
	    .def( "isDesc", &barzer::exposed::BarzerRange::isDesc )
        ;
    boost::python::class_<barzer::exposed::BarzerEntityRangeCombo>( "ERC", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def_readonly( "ent", &barzer::exposed::BarzerEntityRangeCombo::mainEntity )
        .def_readonly( "unit", &barzer::exposed::BarzerEntityRangeCombo::unitEntity )
        .def_readonly( "rng", &barzer::exposed::BarzerEntityRangeCombo::range )
    ;
    boost::python::class_<barzer::exposed::TraceInfo>( "Trace", no_init )
        .def( self_ns::repr(self_ns::self))
        .def( self_ns::str(self_ns::self))
        .def( "emit", &barzer::exposed::TraceInfo::emit )
        .def( "statement", &barzer::exposed::TraceInfo::statement )
        .def( "grammar", &barzer::exposed::TraceInfo::grammar )
        .def( "source", &barzer::exposed::TraceInfo::source )
        ;

	namespace at = barzer::autotester;

	boost::python::class_<at::BeadMatchOptions>("BeadMatchOptions")
		.def("matchType", &at::BeadMatchOptions::matchType)
		.def("setMatchType", nonReturn<at::BeadMatchOptions, at::BeadMatchType, &at::BeadMatchOptions::setMatchType>)
		.def("classMatchType", &at::BeadMatchOptions::classMatchType)
		.def("setClassMatchType", nonReturn<at::BeadMatchOptions, at::ClassMatchType, &at::BeadMatchOptions::setClassMatchType>)
		.def("skipFluff", &at::BeadMatchOptions::skipFluff)
		.def("setSkipFluff", nonReturn<at::BeadMatchOptions, bool, &at::BeadMatchOptions::setSkipFluff>);

	boost::python::class_<at::CompareSettings>("CompareSettings")
		.def("setBeadOptions", &at::CompareSettings::setBeadOption);
}
