#include <zurch_phrasebreaker.h>
#include <zurch_docidx.h>
namespace zurch {

/// returns beginning of the delimiter in case of a match or 0 otherwise
inline const char*  PhraseDelimiterData::match( const char* s, const char* s_end )  const
{
    if( delim.length() == 1 ) 
        return( s< s_end && delim[0] == *s && (!mustBeFollowedBySpace || (s+1<s_end && isspace(s[1]))) ? s:0);
    else if( delim.length() < s_end-s ) 
        return ( !strncmp( delim.c_str(), s, delim.length() ) && (!mustBeFollowedBySpace || (s+1<s_end && isspace(s[1]))) ? s : 0 );
    else 
        return 0;
}
void PhraseDelimiterData::addNoPreceed( const char* s ) { cantBePrecededBy.insert( std::string(s) ); }
void PhraseDelimiterData::addNoPreceed( const char** s, size_t s_sz  ) 
    { for( const char** x = s, **x_end = s+s_sz; x< x_end && *x; ++x  ) addNoPreceed(*x); }

PhraseBreaker::PhraseBreaker( ) : 
        buf( zurch::DocFeatureLoader::DEFAULT_BUF_SZ ), 
        d_breakOnPunctSpace(true) {}

const PhraseDelimiterData* PhraseBreaker::getDelimiterData( const char* d ) const
{
    for( auto i = d_delim.begin(); i!= d_delim.end(); ++i ) {
        if( i->checkDelim(d) ) 
            return &(*i);
    }
    return 0;
}
PhraseDelimiterData* PhraseBreaker::getDelimiterData(  const char* d )
    { return const_cast<PhraseDelimiterData*>( const_cast<const PhraseBreaker*>(this)->getDelimiterData(d) ); }

PhraseDelimiterData* PhraseBreaker::addDelimiterData( const char* d, bool spaceMustFollow) 
    {
        PhraseDelimiterData* delData = getDelimiterData(d);
        if( delData )  {
            if( delData->mustBeFollowedBySpace != spaceMustFollow ) 
                delData->mustBeFollowedBySpace = spaceMustFollow;
            return delData;
        } else {
            d_delim.push_back( PhraseDelimiterData(d,spaceMustFollow) );
            return &(d_delim.back());
        }
    }

void PhraseBreaker::addSingleCharDelimiters( const char * delim, bool spaceMustFollow) 
{
    for( const char* s = delim; *s; ++s ) {
        char x[2] = { *s, 0 };
        addDelimiterData( x, spaceMustFollow );
    }
}

const char* PhraseBreaker::whereToBreak( const char* s, const char* s_beg, const char* s_end )  const
{    
    char c=*s;
    if( c  == '\n' || c == '\r' || c=='(' || c=='|' || c==')' || c=='\t' || c ==';' || (c ==' '&&s[1]=='.'&&s[2]==' ') )
        return s;
        
    std::string tmp;

    for( auto i = d_delim.begin(); i!= d_delim.end(); ++i ) {
        if( const char* delimStr = i->match(s, s_end) ) {
            auto precedeBeg = i->cantBePrecededBy.begin();
            const char* x = delimStr-1;
            for( ; x>= s_beg; --x ) {
                tmp.assign( x, delimStr-x );
                auto pi = precedeBeg;
                for( ; pi != i->cantBePrecededBy.end() && *pi < tmp; ++pi );
                if( pi != i->cantBePrecededBy.end() ) {
                    if( pi->length() == tmp.length() && *pi == tmp )
                        return 0;
                } else
                    break;
            }
            return delimStr;
        }
    }
    return 0;
}

} // namespace zurch 
