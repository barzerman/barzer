#include <barzer_shellsrv_shared.h>

#include <ay/ay_headers.h>
#include <ay/ay_shell.h>
#include <barzer_dtaindex.h>
#include <set>
#include <barzer_parse.h>
//#include <barzer_parse_types.h>
#include <barzer_barz.h>
#include <barzer_universe.h>
#include <barzer_server_response.h>
#include <barzer_server.h>
#include <batch/barzer_batch_processor.h>

namespace barzer {

int BarzerShellSrvShared::batch_parse( Barz& barz, const std::string& url, const std::string& uri, bool fromShell  )
{
    BatchProcessorSettings settings( d_gp );
    uint32_t uniId = 0;
    
    if( !fromShell  ) {
        if( url.empty() )
            return 1;
    } 
    if( !url.empty() ) {
        ay::uri_parse argParser;
        argParser.parse( url );
        for( const auto& i : argParser.theVec ) {
            if( i.first == "id" ) {
                uniId = atoi( i.second.c_str() ) ;
                if( !settings.setUniverseById( uniId ) ) {
                    std::cerr << "invalid universe \"" << i.second << std::endl;
                    return 0;
                }
            } else 
            if( i.first == "in" ) {
                if( auto fp = settings.setInFP(i.second.c_str()) ) {
                    if( !fp->is_open() )  {
                        std::cerr << "cant open input file \"" << i.second << "\"" << std::endl;
                        return 0;
                    }
                }
            } else
            if( i.first == "out" ) {
                if( auto fp = settings.setOutFP(i.second.c_str()) ) {
                    if( !fp->is_open() )  {
                        std::cerr << "cant open output file \"" << i.second << "\"" << std::endl;
                        return 0;
                    }
                }
            }
        }
    }
    BatchProcessorZurchPhrases processor( barz );
    processor.run(settings);
    return 0;
}

} // namespace barzer
