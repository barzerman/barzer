#pragma  once
#include <string>
#include <iostream>
#include <vector>
#include <ay_util.h>
/// char string utilities 
namespace ay {
/// ensures that stream receives properly escaped XML
struct XMLStream {
    std::ostream& os;

    std::ostream& escape(const char *s );
    std::ostream& escape(const char *s, size_t s_sz );
	std::ostream& escape(const std::string&);
	
    XMLStream( std::ostream& o ) : os(o) {}
    template <typename T>
    XMLStream& print( const T& x)
        { return (os<< x, *this ); }
    
    XMLStream& print( const std::string& x)
        { return (escape(x.c_str()), *this); }

    typedef std::ios_base& ( *IOS_BASE_PF )(std::ios_base&);
    typedef std::ios& ( *IOS_PF )(std::ios&);
    typedef std::ostream& ( *IOS_OSTREAM )(std::ostream&);
};

template <typename T>
inline XMLStream& operator<<( XMLStream& s, const T& x )
    { return s.print(x); }
inline XMLStream& operator <<(XMLStream& vs, XMLStream::IOS_OSTREAM pf )
    { return( pf((vs.os)), vs ); }

inline XMLStream& operator<<(XMLStream& vs, XMLStream::IOS_PF pf )
    { return( pf((vs.os)), vs ); }

inline XMLStream& operator<<(XMLStream& vs, XMLStream::IOS_BASE_PF pf ) 
{ return (pf((vs.os)),vs); }

struct tag_raii {
	std::ostream &os;
	std::vector<const char*> tags;

	tag_raii(std::ostream &s) : os(s) {}
	tag_raii(std::ostream &s, const char *tag, const char* attr = 0) : os(s) 
        { push(tag,attr); }
	operator std::ostream&() { return os; }

	void push(const char *tag, const char* attr=0 ) {
		os << "<" << tag << ( attr ? attr : "" ) << ">";
		tags.push_back(tag);
	}

	~tag_raii() {
		size_t i = tags.size();
		while(i--) {
			os << "</" << tags.back() << ">";
			tags.pop_back();
		}
	}
};

struct json_raii {
    std::ostream& d_fp;
    size_t d_count, d_depth;

    bool d_isArray; /// when false this is an object

    bool isArray() const { return d_isArray; }
    bool isObject() const { return !d_isArray; }
    std::ostream& getFP() { return d_fp; }

    json_raii( std::ostream& fp, bool arr, size_t depth ) : 
        d_fp(fp), d_count(0), d_depth(depth),d_isArray(arr)  
        { d_fp << ( isArray() ? "[" : "{" ); }
    
    ~json_raii() 
        { d_fp << ( isArray() ? "]" : "}" ); }
    
    std::ostream& indent(std::ostream& fp) 
    {
        for( size_t i=0; i< d_depth; ++i ) fp << "    ";
        return fp;
    }
    std::ostream& startFieldNoindent( const char* f="" ) 
    {
        d_fp << (d_count++ ? ", ": "");
        return ( isArray() ? d_fp : ay::jsonEscape( f, d_fp<< "\"") << "\": " );
    }
    std::ostream& startField( const char* f, bool noNewLine=false ) 
    {
        if( !d_count && !noNewLine)
            d_fp << "\n";
        indent( d_fp << (d_count++ ? (noNewLine ? ",":",\n"):"") );
        return ( isArray() ? d_fp : ay::jsonEscape( f, d_fp<< "\"") << "\": " );
    }

    std::ostream& addKeyVal( const char* k, const char* v ) 
        { return startField(k) << "\"" << v << "\""; }
    std::ostream& addKeyValNoIndent( const char* k, const char* v ) 
        { return startFieldNoindent(k) << "\"" << v << "\""; }
    size_t getDepth() const { return d_depth; }
};
} // ay namespace ends 
