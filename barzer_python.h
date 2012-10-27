#ifndef BARZER_PYTHON_H
#define BARZER_PYTHON_H

#include <string>
#include <vector>
#include <ay/ay_cmdproc.h>
#include <ay_translit_ru.h>
#include <boost/python.hpp>


typedef boost::python::list boost_python_list;
typedef boost::python::object boost_python_object;

namespace barzer {
class GlobalPools;
struct BarzerShell;
struct QueryParseEnv;
struct PythonQueryProcessor;

namespace autotester
{
	class CompareSettings;
}

namespace boost { namespace python { class list; } } 
//namespace ay { class CommandLineArgs; }

class PythonCmdLine {
    std::vector< std::string > d_argv_str;
    ay::CommandLineArgs*    d_cmdlProc;
    std::vector< const char*> argv;
public:
    PythonCmdLine();
    ~PythonCmdLine();
    ay::CommandLineArgs& cmdlProc() { return *d_cmdlProc; }
    
    /// must be always called after the last addArg
    PythonCmdLine& init( boost_python_list & ns );
};

/// python exportable class 
class BarzerPython {
protected:
    GlobalPools* gp;
    BarzerShell* shell;
    
    /// these two are tightly coupled

    PythonCmdLine       d_pythCmd;
    StoredUniverse*     d_universe;
    QueryParseEnv*      d_parseEnv;
public:
    // gp is never null
    const GlobalPools& getGP() const { return *gp; }
    GlobalPools& getGP() { return *gp; }


    StoredUniverse* getUniversePtr() { return d_universe; }
    StoredUniverse& guaranteeUniverse() { if( !d_universe) d_universe=gp->getUniverse(0); return *d_universe; }

    const StoredUniverse* getUniversePtr() const { return d_universe; }

    BarzerPython();
    ~BarzerPython();

    //// functions exposed to Python
    int             init( boost_python_list& ns );
    std::string     parse( const std::string& q );
    int             setUniverse( const std::string& us );
    std::string     emit(const std::string& q );
    //std::string     count_emit(const std::string& q ) const;
    /// exported functions 
    std::string     bzstem(const std::string& s);
    boost_python_list guessLang(const std::string& s);

    int matchXML(const std::string& pattern, const std::string& result, const autotester::CompareSettings&);
	
	std::string translitEn2Ru(const std::string& from/*, const ay::tl::TLExceptionList_t&*/);
	std::string translitRu2En(const std::string& from/*, const ay::tl::TLExceptionList_t&*/);

    void shell_cmd( const std::string& cmd, const std::string& args );

    PythonQueryProcessor*  makeParseEnv( ) const;
};


}
#endif // BARZER_PYTHON_H 
