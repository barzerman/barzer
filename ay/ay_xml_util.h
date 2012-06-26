#ifndef AY_XML_UTIL_H
#define AY_XML_UTIL_H

#include <string>
#include <iostream>
/// char string utilities 
namespace ay {
/// ensures that stream receives properly escaped XML
struct XMLStream {
    std::ostream& os;

    std::ostream& escape(const char *s );
    std::ostream& escape(const char *s, size_t s_sz );
    XMLStream( std::ostream& o ) : os(o) {}
    template <typename T>
    XMLStream& print( const T& x)
        { return (os<< x, *this ); }
    
    XMLStream& print( const char* x)
        { return (escape(x), *this); }
    XMLStream& print( const std::string& x)
        { return (escape(x.c_str()), *this); }
};
template <typename T>
inline XMLStream& operator<<( XMLStream& s, const T& x )
    { return s.print(x); }

inline XMLStream& operator<<(XMLStream& vs, std::ostream& ( *pf )(std::ostream&)) 
    { return( pf((vs.os)), vs ); }

inline XMLStream& operator<<(XMLStream& vs, std::ios& ( *pf )(std::ios&)) 
    { return( pf((vs.os)), vs ); }

inline XMLStream& operator<<(XMLStream& vs, std::ios_base& ( *pf )(std::ios_base&)) 
{ return (pf((vs.os)),vs); }

} // ay namespace ends 
#endif // AY_XML_UTIL_H 
