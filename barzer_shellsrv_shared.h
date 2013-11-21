#pragma once

#include <string>
namespace barzer {
class GlobalPools;
class Barz;
/// functions exposed to both the tcp service  (!!XXX prefix) and 
/// the shell. 
/// function bodies in shell_0xx files
struct  BarzerShellSrvShared {
    const GlobalPools& d_gp;

    BarzerShellSrvShared( const GlobalPools& gp  ) : 
        d_gp(gp) {}
    
    int batch_parse( Barz& barz, const std::string& url, const std::string& uri, bool fromShell  );
};

} // namespace barzer
