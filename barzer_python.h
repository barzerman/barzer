#ifndef BARZER_PYTHON_H
#define BARZER_PYTHON_H

#include <string>
#include <vector>
#include <ay/ay_cmdproc.h>

typedef boost::python::list boost_python_list;

namespace barzer {
class GlobalPools;
struct BarzerShell;

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

    PythonCmdLine d_pythCmd;
    StoredUniverse* d_universe;
public:
    BarzerPython();
    ~BarzerPython();

    int init( boost_python_list& ns );
    
    void shell_cmd( const std::string& cmd, const std::string& args );

    /// exported functions 
    std::string bzstem(const std::string& s);
};


}
#endif // BARZER_PYTHON_H 
