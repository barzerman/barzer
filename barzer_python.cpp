#include <barzer_universe.h>
#include <barzer_shell.h>
#include <boost/python.hpp>
#include <barzer_python.h>
#include <ay/ay_cmdproc.h>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>

#include <barzer_parse.h>
#include <barzer_server_response.h>

#include <util/pybarzer.h>

using boost::python::stl_input_iterator ;
using namespace boost::python;

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


} // barzer

BOOST_PYTHON_MODULE(pybarzer)
{
    boost::python::class_<barzer::BarzerPython>( "Barzer" )
        .def( "stem", &barzer::BarzerPython::bzstem )
        .def( "init", &barzer::BarzerPython::init )
        .def( "universe", &barzer::BarzerPython::setUniverse )
        .def( "parse", &barzer::BarzerPython::parse  );
    
    def("stripDiacritics", stripDiacritics);
    // boost::python::class_<barzer::BarzerPython>( "Entity" )
}
