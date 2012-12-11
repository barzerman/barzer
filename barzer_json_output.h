#pragma once

#include <ay/ay_util.h>
#include <barzer_server_response.h>

namespace barzer {

struct JSONRaii {
    std::ostream& d_fp;
    size_t d_count, d_depth;

    bool d_isArray; /// when false this is an object

    bool isArray() const { return d_isArray; }
    bool isObject() const { return !d_isArray; }

    JSONRaii( std::ostream& fp, bool arr, size_t depth ) : 
        d_fp(fp), d_count(0), d_depth(depth),d_isArray(arr)  
        { d_fp << ( isArray() ? "[" : "{" ); }
    
    ~JSONRaii() 
        { d_fp << ( isArray() ? "]" : "}" ); }
    
    std::ostream& indent(std::ostream& fp) 
    {
        for( size_t i=0; i< d_depth; ++i ) fp << "    ";
        return fp;
    }
    std::ostream& startFieldNoindent( const char* f="" ) 
    {
        d_fp << (d_count++ ? "": ", ");
        return ( isArray() ? d_fp : ay::jsonEscape( f, d_fp<< "\"") << "\":" );
    }
    std::ostream& startField( const char* f="" ) 
    {
        indent( d_fp << (d_count++ ? "": ",\n") );
        return ( isArray() ? d_fp : ay::jsonEscape( f, d_fp<< "\"") << "\":" );
    }
    size_t getDepth() const { d_depth; }
};

class BarzStreamerJSON : public BarzResponseStreamer {
public:
	BarzStreamerJSON(const Barz &b, const StoredUniverse &u) : BarzResponseStreamer(b, u) {}
	BarzStreamerJSON(const Barz &b, const StoredUniverse &u, const ModeFlags& mf) : 
        BarzResponseStreamer(b, u), d_outputMode(mf) 
    {}

	std::ostream& printConfidence(std::ostream&);
	std::ostream& print(std::ostream&);
};


} // namespace barzer 
