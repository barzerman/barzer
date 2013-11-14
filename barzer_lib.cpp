#include <barzer_lib.h>
#include <ay/ay_cmdproc.h>
#include <barzer_universe.h>
#include <barzer_server_request.h>

namespace barzer {
namespace barzerlib {

namespace {

struct Instance_Internal {
    ay::CmdLineArgsContainer arg;
    GlobalPools gp;
    
    Instance_Internal& init( const std::vector< std::string >& a )
    {
        arg.appendArg( a );
        gp.init_cmdline( arg.cmd );
        return *this;
    }
};

} // anonymous napespace


Instance::Instance()
{
    d_data = new Instance_Internal();
}
Instance::~Instance()
{
    delete static_cast<Instance_Internal*>(d_data);
}

Instance& Instance::init( const std::vector< std::string >& argv )
{
    static_cast<Instance_Internal*>(d_data)->init( argv );
    return *this;
}

Instance& Instance::run_shell()
{
    return *this;
}
Instance& Instance::run_server(uint16_t port)
{
    return *this;
}

int Instance::processUri( std::ostream& outFP, const char* x )
{
    std::string bufStr;
    ay::url_encode( bufStr, x, strlen(x) ); 
    const char* buf = bufStr.c_str();
    std::stringstream outSstr;
    const Instance_Internal& internal = static_cast<Instance_Internal*>(d_data)[ 0 ];
    const GlobalPools& gp = internal.gp;
    BarzerRequestParser reqParser(gp,outSstr);
    const char* firstSlash = strchr( buf, '/' );
    std::string uriStr, queryStr;
    if( firstSlash ) {
        char* firstQuestionMark = strchr( firstSlash, '?' );
        if( firstQuestionMark ) {
            uriStr= std::string( firstSlash, firstQuestionMark-firstSlash );
            queryStr = std::string( firstQuestionMark+1, strlen(firstQuestionMark+1) );
        } 
    }
    if( !uriStr.length() || !queryStr.length() ) 
        return URIERR_BAD_URI;
        
    std::string uri;
    ay::url_encode( uri, uriStr.c_str(), uriStr.length() );
    std::string query;
    ay::url_encode( query, queryStr.c_str(), queryStr.length() );
    barzer::QuestionParm qparm;
    if( !reqParser.initFromUri( qparm, uri.c_str(), uri.length(), query.c_str(), query.length() ) )
        reqParser.parse(qparm);
    
    outFP << outSstr.str() << std::endl;

    return URIERR_ERR_OK;
}

} // namespace barzerlib
} // namespace barzer 
