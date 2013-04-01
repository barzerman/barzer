#include <barzer_question_parm.h>
namespace barzer {

bool QuestionParm::AutocParm::entFilter( const StoredEntityUniqId& euid ) const 
{
    if( ecVec.size() ) {
        for( std::vector< StoredEntityClass >::const_iterator i = ecVec.begin(); i!= ecVec.end(); ++i ) { 
            if( i->matchOther(euid.eclass) )
            return true;
        }
        return false;
    } else 
        return true; 
}

int16_t QuestionParm::parseBeniFlag( const char* s ) 
{
    if( !s )
        return BENI_DEFAULT;
    switch( *s ) {
    case 'b': return BENI_BENI_ONLY_DEFAULT;
    case 'h': return BENI_BENI_ONLY_HEURISTIC;
    case 'e': return BENI_BENI_ENSURE;
    case 'n': return BENI_NO_BENI;
    case 'd': 
    default:
    return BENI_DEFAULT;
    }
}

void QuestionParm::AutocParm::parseEntClassList( const char* s)
{
    const char* b = s, *pipe = strchr(b,'|'), *b_end = b+ strlen(b);
    StoredEntityClass ec;
    for( ; b &&*b; b= (pipe?pipe+1:0), (pipe = (b? strchr(b,'|'):0)) ) {
        ec.ec = atoi(b);
        const char *find_end = (pipe?pipe: b_end), *comma = std::find( b, find_end, ',' );
        ec.subclass = ( comma!= find_end ? atoi(comma+1): 0 );
        if( ec.isValid() ) ecVec.push_back( ec );
    }
}
} // namespace barzer
