
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <set>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <boost/property_tree/ptree.hpp>

namespace zurch {

struct compare_string_rev_less {
bool operator()( const std::string& l, const std::string& r ) const
{
    std::string::const_reverse_iterator li = l.rbegin(), ri = r.rbegin();
    for( ; li != l.rend() && ri != r.rend() && *li==*ri; ++li, ++ri );
    
    if( li == l.rend() ) 
        return !( ri == r.rend() );
    else if( ri == r.rend() ) 
        return false;
    else 
        return *li < *ri;
}

};

struct PhraseDelimiterData {

    std::string delim;
    std::set< std::string, compare_string_rev_less > cantBePrecededBy;
    bool mustBeFollowedBySpace; // default false

    PhraseDelimiterData( const char* c, bool ms=false ) : delim(c), mustBeFollowedBySpace(ms) {}
    /// returns beginning of the delimiter in case of a match or 0 otherwise
    const char*  match( const char* s, const char* s_end )  const;
    void addNoPreceed( const char* s );
    void addNoPreceed( const char** s, size_t s_sz  ) ;

    inline bool checkDelim( const char* s) const
    {
        if( delim.length() == 1 )
            return( !s[1] && s[0] == delim[0] ) ;
        else 
            return delim == s;
    }
};

/// this gets used by PhraseBreaker to keep the state of phrasebreaking. 
/// it keeps track of things such as whether the current phrase is ina  heading etc
struct PhraseBreakerState {
   int phraseBoost;  // 0 - default
   PhraseBreakerState() : phraseBoost(0) {}
    void clear() { phraseBoost= 0; }
};

struct PhraseBreaker {
    std::vector< char > buf;
    
    std::vector<PhraseDelimiterData> d_delim;

    std::string                      d_extraSingleCharDelim;

    typedef std::vector< std::string > string_vec_t;

    std::map< std::string, string_vec_t > d_noBreak;

    bool d_breakOnPunctSpace; /// when true 
    PhraseBreakerState state;

    size_t d_numBuffers, // total number of buffers 
           d_numPhrases; // total number of phrases

    PhraseBreaker( ) ;

    void clear();
    const PhraseDelimiterData* getDelimiterData( const char* d ) const;
    PhraseDelimiterData* getDelimiterData(  const char* d );
    PhraseDelimiterData* addDelimiterData( const char* d, bool spaceMustFollow=false ) ;

    void addSingleCharDelimiters( const char * delim, bool spaceMustFollow=true ) ;

    bool breakOnPunctuation( const char* s ) 
        { return( d_breakOnPunctSpace && ( ispunct(*s) && s[1] ==' ' ) ); }

    const char* whereToBreak( const char* s, const char* s_beg, const char* s_end )  const;
    // breaks the buffer into phrases . callback is invoked for each phrase
    // phrase delimiter is a new line character 
    // returns the size  of the processed text
    template <typename CB>
    size_t breakBuf( CB& cb, const char* str, size_t str_sz ) 
    {
        ++d_numBuffers;
        const char *s_to = str;
		const char *s_from = str;
        for( const char *s_end = str + str_sz; s_to < s_end && s_from< s_end; ) {
            char c = *s_to;
            const char* newTo = 0;
            if( c=='\n' || (isspace(c) && c!=' ')|| 
                ( (ispunct(c)||isspace(c)) && whereToBreak(s_to, str, s_end ) )
            ) {
                if( s_to> s_from ) {
                    cb( *this, s_from, s_to-s_from );
                    ++d_numPhrases;
                    state.clear();
                }
                s_from = ++s_to;
            } else
                ++s_to;
        }
        if (str == s_from)
			cb(*this, s_from, s_to - s_from);
        return s_to - str;
    }

    template <typename CB>
    void breakStream( CB& cb, std::istream& fp ) 
    {
        size_t curOffset = 0;
        while( true ) {
            size_t bytesRead = fp.readsome( &(buf[curOffset]), (buf.size()-curOffset-1) );
            if( !bytesRead )   
                return;
            if( bytesRead < buf.size() ) {
                buf[ bytesRead ] = 0;
                size_t endOffset = breakBuf(cb, &(buf[curOffset]), bytesRead );
                if( endOffset < bytesRead ) { /// something is left over
                    for( size_t dest = 0, src = endOffset; src< bytesRead; ++src, ++dest ) 
                        buf[dest] = buf[src];
                } else if( endOffset > bytesRead )
                    curOffset = 0;
                else
                    curOffset= bytesRead-endOffset;
            } 
        }
    }
    //// delimiters file format 

};

} // namespace zurch 
