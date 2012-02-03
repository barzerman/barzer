#include <barzer_universe.h>
#include <barzer_shell.h>
#include <boost/python.hpp>
#include <barzer_python.h>
#include <ay/ay_cmdproc.h>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>

using boost::python::stl_input_iterator ;
using namespace boost::python;

namespace barzer {

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
    d_universe(0)
{}

BarzerPython::~BarzerPython()
{
    delete shell;
    delete gp;
}

int BarzerPython::init( boost_python_list& ns )
{
    gp->init_cmdline( d_pythCmd.init(ns).cmdlProc() );
    d_universe = gp->getUniverse(0);
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

} // barzer

BOOST_PYTHON_MODULE(pybarzer)
{
    boost::python::class_<barzer::BarzerPython>( "Barzer" )
        .def( "stem", &barzer::BarzerPython::bzstem )
        .def( "init", &barzer::BarzerPython::init );
}
