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

    typedef std::ios_base& ( *IOS_BASE_PF )(std::ios_base&);
    typedef std::ios& ( *IOS_PF )(std::ios&);
    typedef std::ostream& ( *IOS_OSTREAM )(std::ostream&);
};
template <typename T>
inline XMLStream& operator<<( XMLStream& s, const T& x )
    { return s.print(x); }
inline XMLStream& operator<<( XMLStream& s, const char* x )
    { return s.print(x); }

inline XMLStream& operator <<(XMLStream& vs, XMLStream::IOS_OSTREAM pf )
    { return( pf((vs.os)), vs ); }

inline XMLStream& operator<<(XMLStream& vs, XMLStream::IOS_PF pf )
    { return( pf((vs.os)), vs ); }

inline XMLStream& operator<<(XMLStream& vs, XMLStream::IOS_BASE_PF pf ) 
{ return (pf((vs.os)),vs); }

} // ay namespace ends 
#endif // AY_XML_UTIL_H 
