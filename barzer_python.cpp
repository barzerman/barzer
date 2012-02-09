#include <barzer_universe.h>
#include <barzer_shell.h>
#include <boost/python.hpp>
#include <barzer_python.h>
#include <ay/ay_cmdproc.h>
#include <boost/python/list.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>

#include <barzer_parse.h>
#include <barzer_server_response.h>
#include <barzer_el_chain.h>
#include <boost/variant.hpp>

#include <util/pybarzer.h>


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
    for (int i = 0; i < len(ns); ++i) {
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

void BarzerPython::shell_cmd( const std::string& cmd, const std::string& args )
{
    if( !shell ) 
        shell = new BarzerShell(0,*gp);
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
    if( d_universe ) {
        if( d_parseEnv ) {
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

struct foo {
    std::string id;
    // void init(const std::string& s ) { id = s; }
    void init() { id = "myid1"; }
}; 

struct bar {
    dict d;
    list l;
    void init() {
        foo foo; 
        foo.init();
        d[ "foo" ] = foo;
        l.append( foo );
    }
    object f() const { return d.get("foo"); }
}; 

//// encoding visitors and service functions 
namespace {


namespace exposed {

struct BarzerLiteral {
    barzer::BarzerLiteral ltrl;
    std::string txt;
    
    BarzerLiteral( const barzer::BarzerLiteral& l, const StoredUniverse& u ) :
        ltrl(l), txt(u.printableStringById(l.getId()))
    {}
};

} // exposed namespace 

namespace visitor {

struct bead_list : public static_visitor<bool> {
    const StoredUniverse&   d_universe;
    const BarzelBead&       d_bead; 
    list&                   d_list;
    size_t                  d_beadNum;
     
    bead_list( list& l, const StoredUniverse& u, const BarzelBead& b, size_t beadNum ) :
        d_universe(u), d_bead(b), d_list(l), d_beadNum(beadNum)
    {}
    
    bool operator()( const BarzerString& l )
        { return (d_list.append( l.getStr() ),true); }
    bool operator()( const BarzerLiteral& l )
    {
        if( l.isString() ) {
            d_list.append( std::string(d_universe.printableStringById(l.getId())) );
        } else {
            d_list.append(exposed::BarzerLiteral(l,d_universe));
        }
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

} // encode_visitor namespace ends

} //  anonymous namespace ends

/// 
struct BarzerResponseObject {
    list beadList;  /// list of beads  

    list topicInfo; /// information on the topics discovered during parse 

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
        /* uncomment and try compiling 
        const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
        if( !topicMap.empty() ) {
            BarzelBead fakeBead;
            EncoderBeadVisitor v(os, universe, fakeBead );
            for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
                std::stringstream sstr;
                sstr << "strength=\"" << topI->second << "\"";
                v.printEntity( topI->first, sstr.str().c_str() );
            }
        }


        /// printing spell corrections  if any 
        if( spellCorrections.size( ) ) {
            for( CToken::SpellCorrections::const_iterator i = spellCorrections.begin(); i!= spellCorrections.end(); ++i ) {
                os << "<correction before=\"" << i->first << "\" after=\"" << i->second << "\"/>\n";
            }
        }
        printTraceInfo(os, barz, universe);
        */
    }
};

struct PythonQueryProcessor {
    const BarzerPython& bpy;
    Barz d_barz;

    PythonQueryProcessor(const BarzerPython&b): 
        bpy(b)
        {}
    
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

BOOST_PYTHON_MODULE(pybarzer)
{
    boost::python::class_<barzer::BarzerPython>( "Barzer" )
        .def( "stem", &barzer::BarzerPython::bzstem )
        .def( "init", &barzer::BarzerPython::init )
        .def( "universe", &barzer::BarzerPython::setUniverse )
        .def( "mkProcessor", &barzer::BarzerPython::makeParseEnv, return_value_policy<manage_new_object>() )
        /// returns Barzer XML for the parsed query 
        .def( "parsexml", &barzer::BarzerPython::parse  );
    
    def("stripDiacritics", stripDiacritics);
    // BarzerResponseObject    
    boost::python::class_<barzer::BarzerResponseObject>( "response" )
        .def_readwrite( "beads", &barzer::BarzerResponseObject::beadList );

    // boost::python::class_<barzer::BarzerPython>( "Entity" )
    boost::python::class_<barzer::PythonQueryProcessor>( "processor", no_init )
        .def( "parse", &barzer::PythonQueryProcessor::parse, return_value_policy<manage_new_object>() )
        .def( "parsexml", &barzer::PythonQueryProcessor::parseXML );

    boost::python::class_<barzer::bar>( "bar" )
        .def_readwrite( "d", &barzer::bar::d )
        .def_readwrite( "l", &barzer::bar::l )
        .def( "init", &barzer::bar::init );

    boost::python::class_<barzer::foo>( "foo" )
        .def_readwrite( "d", &barzer::foo::id )
        .def( "init", &barzer::foo::init );

    boost::python::class_<barzer::exposed::BarzerLiteral>( "Token", no_init )
        .def_readonly( "txt", &barzer::exposed::BarzerLiteral::txt );
}
