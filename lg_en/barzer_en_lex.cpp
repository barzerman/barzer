#include <lg_en/barzer_en_lex.h>
namespace barzer {
int QSingleLangLexer_EN::lex( CTWPVec& , const TTWPVec&, const QuestionParm& )
{
	return 0;
}

namespace {

typedef std::pair< const char*, const char* > char_cp_pair;

struct char_cp_pair_less {
    inline bool operator()( const char_cp_pair& l, const char_cp_pair& r ) 
        { return (strcmp(l.first,r.first)< 0); }
};

/// make sure the list is ordered alphabetically by the first terms in the pairs 
char_cp_pair g_plur[] = {
    char_cp_pair( "corpora", "corpus" ),
    char_cp_pair( "criteria", "criterion" ),
    char_cp_pair( "indices", "index" ),
    char_cp_pair( "matrices", "matrix" ),
    char_cp_pair( "movies", "movie" )
};

} // anonymous namespace

namespace ascii {
const char* english_exception_depluralize( const char* w, size_t w_len ) 
{
    char buf[ 32 ];
    if( w_len > (sizeof(buf)-1) )
        w_len = (sizeof(buf)-1) ;

    strncpy( buf, w, w_len );
    buf[w_len] = 0;

    const char_cp_pair* i = std::lower_bound( 
        g_plur, ARR_END(g_plur), char_cp_pair(buf,0), char_cp_pair_less()
    );
    if( i != ARR_END(g_plur) && !strcasecmp(i->first,w) ) {
        return i->second;
    } else
        return 0;
}

} // namespace ascii

} // anon namespace
