
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <ay/ay_cmdproc.h>
#include <barzer_universe.h>


namespace barzer {

class BarzerHttpServer {
public:
    GlobalPools& gp;
private:
    BarzerHttpServer( GlobalPools& g ) : gp(g) {}
public:
    
    static int run( const ay::CommandLineArgs& cmd, int argc, char* argv[] );
    
    static void destroyInstance();
    static BarzerHttpServer& mkInstance( GlobalPools& g );
    static BarzerHttpServer& instance( );
};

} // namespace barzer
