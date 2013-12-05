#pragma once
#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <vector>
#include <ay_utf8.h>
/// char string utilities 
namespace ay {

template <typename T>
struct char_compare { bool operator()( const T& l, const T& r ) const { return (l==r); } };

struct char_compare_nocase_ascii { 
	bool operator()( char l, char r ) const { return( toupper(l) == toupper(r) ); } 
};

// 2 byte char 
struct Char2B {
    char b[2];

    Char2B( ) { b[0] = b[1] = 0; }
    Char2B( const char* s ) { b[0] = *s; b[1] = s[1]; }
    
    inline static void mkCharvec( std::vector<char>& out, const std::vector< Char2B >& cv )
    {
        out.clear();
        out.reserve( 2*cv.size() + 1 );
        for( std::vector< Char2B >::const_iterator i = cv.begin(); i!= cv.end(); ++i ) {
            out.push_back( i->b[0] );
            out.push_back( i->b[1] );
        }
        out.push_back( 0 );
    }
    inline static void mkCharvec( 
        std::vector<char>& out, 
        std::vector< Char2B >::const_iterator& beg ,
        std::vector< Char2B >::const_iterator& end
        )
    {
        out.clear();
        out.reserve( 2*(end-beg) + 1 );
        for( std::vector< Char2B >::const_iterator i = beg; i!= end; ++i ) {
            out.push_back( i->b[0] );
            out.push_back( i->b[1] );
        }
        out.push_back( 0 );
    }
};

inline bool operator==( const Char2B& l, const Char2B& r ) { return( l.b[0] == r.b[0] && l.b[1] == r.b[1]); }
inline bool operator!=( const Char2B& l, const Char2B& r ) { return !(l == r ); }

inline bool operator<( const Char2B& l, const Char2B& r ) 
    { return( l.b[0] < r.b[0] || ( l.b[0] == r.b[0] && (l.b[1] < r.b[1]) ) ); }

struct Char2B_iterator {
    const char* d_s;
    Char2B_iterator( ): d_s(0) {}
    Char2B_iterator( const char* s ): d_s(s) {}
    Char2B_iterator& operator++() { d_s += 2; return *this; }
    Char2B_iterator& operator--() { d_s -= 2; return *this; }
    Char2B_iterator& operator+( int i) { d_s += (2*i); return *this; }
    Char2B_iterator& operator-( int i) { d_s -= (2*i); return *this; }

    Char2B operator *() { return Char2B(d_s); }

    const char* operator[]( size_t i ) const { return ( d_s + (i*2) ); }
};
inline int operator -( const Char2B_iterator& l, const Char2B_iterator& r ) 
    { return ( (l.d_s - r.d_s)/2 ); }

inline bool operator!= (const Char2B_iterator& l, const Char2B_iterator& r ) { return ( l.d_s != r.d_s); }
inline bool operator== (const Char2B_iterator& l, const Char2B_iterator& r ) { return ( l.d_s == r.d_s); }
inline bool operator< (const Char2B_iterator& l, const Char2B_iterator& r ) { return ( l.d_s < r.d_s); }

/// prefix matcher  - pfx should be an array of const char* terminated by 0
/// 
inline bool c_str_match_prefix( const char* r, const char* s )
{
       const char* x = r;
       for( ; *x && *s; ++x, ++s ) 
           if( *x != *s ) return false;
       
       return (*x || !*s);
}
inline const char* c_str_match_any_prefix( const char* s, const char* pfx[] ) 
{
    for( ; *pfx; ++pfx ) { if( c_str_match_prefix(s,*pfx) ) return *pfx; }

    return 0;
}
/// Char2B_accessor str("фанат");
/// str("ф") is true
/// str(1,"a") is true

struct Char2B_accessor {
    const char* d_s;
    
    Char2B_accessor( const char* s) : d_s(s) {}
    Char2B_accessor( const char* s, size_t offset) : d_s(s+offset) {}

    bool isChar( const char* c2b ) const { return ( *d_s == c2b[0] && d_s[1] == c2b[1] ); }
    // s must be 0 terminated
    const char* isOneOfChars( const char* s[] ) const { 
        for( ; *s; ++s ) {
            if( isChar(*s) )
                return *s;
        }
        return 0;
    }

    bool isChar( int i, const char* c2b ) const 
    {
        const char* io = ( d_s +  (2*i) );
        return ( *io == c2b[0] && io[1] == c2b[1] ); 
    }
    
    bool prefixMatch( const char* s ) const 
        { 
            const char* x = d_s;
            for( ; *x && *s; ++x, ++s ) 
                { if( *x != *s ) return false; } 
            return (*x || !*s); 
        }
    /// this does prefix match 
    bool operator()( const char* s ) const { return c_str_match_prefix(d_s,s); }
    /// gets array of strings terminated by 0 string
    /// returns true as soon as the first prefix match returns true
    /// const char * a[] = { "a", "b"}
    const char* operator()( const char* s[] ) const { return c_str_match_any_prefix(d_s,s); }

    bool operator==( const char* s ) const { return !strcmp(d_s,s); }

    const char* c_str() const { return d_s; }

    Char2B_accessor offset( int i ) const { return Char2B_accessor( d_s + (2*i) ); }
    Char2B_accessor prev( ) const { return Char2B_accessor(d_s-2); }
    Char2B_accessor next( ) const { return Char2B_accessor(d_s+2); }
    
    char operator *( ) const { return *d_s; }
}; // Char2B_accessor

inline bool is_all_digits( const char* s )
{
    for( const char* x = s; *x; ++x )
        if( !isdigit(*x) )
            return false;
    return true;
}

inline bool is_2bchar_prefix_1( const char* x, const char* p )
    { return ( x[0] == p[0] && x[1] == p[1] ); }

inline bool is_2bchar_prefix_2( const char* x, const char* p )
    { return ( is_2bchar_prefix_1(x,p) && is_2bchar_prefix_1( x+2, p+2 ) ); }
inline bool is_2bchar_prefix_3( const char* x, const char* p )
    { return ( is_2bchar_prefix_2(x,p) && is_2bchar_prefix_1( x+4, p+4 ) ); }
inline bool is_2bchar_prefix_4( const char* x, const char* p )
    { return ( is_2bchar_prefix_2(x,p) && is_2bchar_prefix_2( x+4, p+4 ) ); }

//// end of 2 byte char 

} // ay namespace ends 
