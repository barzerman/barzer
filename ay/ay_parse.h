#ifndef AY_PARSE_H
#define AY_PARSE_H

#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>

namespace ay {

// CB must have the following prototype 
/// int operator()( size_t tok_num, const char* tok, const char* tok_end ) 
template <typename CB>
void parse_separator( CB&cb, const char* s, const char* s_end, char sep='|' )
{
    const char* b = s, *pipe = strchr(b,sep);
    size_t tok_num = 0;
    for( ; b &&(b<s_end) && !cb( tok_num++, b, (pipe?pipe:s_end) ); b= (pipe?pipe+1:0), (pipe = (b? std::find(b,s_end,sep):0)) );
}

class string_tokenizer_iter  {
public:
    typedef std::pair< const char*, const char* > string_range;
private:
    string_range fullRng;
    string_range rng;
    char sep;
public: 

    string_tokenizer_iter( const string_range& r, char sepChar ): 
        fullRng(r),
        rng(r.first,std::find(r.first,r.second,sepChar)), 
        sep(sepChar) { }
    string_tokenizer_iter( const char* s, char sepChar ): 
        fullRng(s,s+strlen(s)),
        rng(s,std::find(s,fullRng.second,sepChar)), 
        sep(sepChar) { }
    string_tokenizer_iter( const char* s, const char* s_end, char sepChar ): 
        fullRng(s,s_end), rng(s,std::find(s,s_end,sepChar)), sep(sepChar) { }
    

    void reset() { 
        rng.first = fullRng.first; 
        rng.second =std::find(fullRng.first,fullRng.second,sep);
    }

    bool notDone() const { return(rng.first &&(rng.first<fullRng.second)); }
    operator bool() const { return notDone(); }

    string_range get_range() const { return rng; }
    bool get_string( std::string& s ) const {
        if( notDone() ) {
            s.assign( rng.first, rng.second-rng.first );
            return true;
        } else {
            s.clear();
            return false;
        }
    }
    void next() 
    {
        if( notDone() ) {
            rng.first= (rng.second?rng.second+1:0); 
            rng.second = (rng.first? std::find(rng.first,fullRng.second,sep):0);
        }  
    }
    void operator++( ) { next(); }
};

struct tupple_callback_data {
    std::vector< std::pair< const char* , const char* > > tokVec;

    int operator()( size_t t_num, const char* t, const char* t_end )
        { tokVec.push_back(  std::pair<const char* , const char*>(t,t_end) ); return 0; }

    void     get_string_field( std::string& s, size_t i ) const
    {
        s.clear();
        if( i< tokVec.size() ) {
            const std::pair< const char* , const char* >& t = tokVec[i];
            s.append( t.first, t.second );
        }
    }
    int      get_int_field( size_t i ) const
    {
        if( i< tokVec.size() ) {
            char b[ 32 ];
            const std::pair< const char* , const char* >& t = tokVec[i];
            size_t len = t.second - t.first;
            if( len> sizeof(b)-1 ) len= sizeof(b)-1;
            strncpy( b, t.first, len );
            return atoi( ((b[sizeof(b)-1]=0),b) );
        } else
            return 0;
    }
    double   get_double_field( size_t i ) const
    {
        if( i< tokVec.size() ) {
            char b[ 32 ];
            const std::pair< const char* , const char* >& t = tokVec[i];
            size_t len = t.second - t.first;
            if( len> sizeof(b)-1 ) len= sizeof(b)-1;
            strncpy( b, t.first, len );
            return atof( ((b[sizeof(b)-1]=0),b) );
        } else
            return 0.0;
    }
};
/// CB - tupple callback 
/// int operator()( const vector< std::pair< const char*, const char* > > )
template <typename CB>
struct parse_separator_tuple_cb {
    CB& cb;
    char innerSep;
    parse_separator_tuple_cb(CB&c, char s = ',' ): 
        cb(c), innerSep(s) {}
    
    struct TuppleDataCB {
        tupple_callback_data& tcd;
        TuppleDataCB(tupple_callback_data& d ) : tcd(d) {}
        int operator()( size_t t_num, const char* t, const char* t_end )
            { 
                tcd.tokVec.push_back( std::pair< const char* , const char* >( t,t_end) ); 
                return 0;
            }
    };

    int operator()( size_t t_num, const char* t, const char* t_end )
    { 
        tupple_callback_data tcd;
        TuppleDataCB tdcb(tcd);
        parse_separator( tdcb, t, t_end, innerSep );
        cb( tcd );
        return 0;
    }
};

template <typename T>
int parse_separator_tupple( T&t , const char* s, char outerSep, char innerSep  ) 
{
    parse_separator_tuple_cb< T > cb( t, innerSep );
    parse_separator( cb, s, s+strlen(s), outerSep );
    return 0;
}

} // ay namespace 


#endif //  AY_PARSE_H
