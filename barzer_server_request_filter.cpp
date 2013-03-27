#include <barzer_server_request_filter.h>
#include <ay/ay_parse.h>
#include <zurch_docidx.h>

namespace barzer {

void ReqFilterCascade::parse( const char* str, const char* str_end )
{
    ReqFilter filter;
    std::string propName;

    ay::parse_separator( 
        [&]( size_t tok_num, const char* tok, const char* tok_end ) -> bool {
            switch( tok_num ) {
            case 0: // Type i - integer, d - double, s - string
                filter.setDataType(tok,tok_end);
                break;
            case 1: // Filter type r - range, o - one of 
                filter.setFilterType(tok,tok_end);
                break;
            case 2: // property name
                if( tok_end> tok ) 
                    propName.assign( tok, tok_end-tok );
                break;
            default: 
                filter.push_back( std::string( tok, tok_end-tok ) );
            }
            return false;
        },
        str,              // buffer
        str_end, /// end of buffer
        ',' /// separator
    );
    if( !propName.empty() && !filter.empty() ) {
        filter.sort();
        addFilter( propName, filter );
    }
}

} // namespace barzer
