#include <ay_xml_util.h>

/// char string utilities 
namespace ay {

namespace {
inline std::ostream& singleCharPut( std::ostream& os, char c )
{
    switch (c) {
    case '&': return os << "&amp;";
    case '<': return os << "&lt;";
    case '>': return os << "&gt;";
    case '"': return os << "&quot;";
    case '\'': return os << "&apos;";
    default: return( os.put(c), os );
    }
}

}

std::ostream& XMLStream::escape(const char *s)
{
	for(;*s; ++s) singleCharPut(os,*s);
	return os;
}
std::ostream& XMLStream::escape(const char *s, size_t s_sz )
{
    const char* s_end = s+s_sz;
	for(;s< s_end; ++s) singleCharPut(os,*s);
	return os;
}

std::ostream& XMLStream::escape(const std::string& str)
{
	return escape(str.c_str(), str.size());
}

} // ay namespace ends 
