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

namespace html {
void unescape_in_place( std::string& str )
{
    size_t pos = 0;
    
    auto knownFixer = [&pos, &str](const std::string& entity, const std::string& after) -> bool
    {
        if (pos + entity.size() + 1 >= str.size())
            return false;
        
        for (size_t i = 0; i < entity.size(); ++i)
        {
            if (entity[i] != str[pos + i + 1])
                return false;
        }
        
        str.replace(pos, entity.size() + 1, after);
			pos += after.size();
        return true;
    };
		
    while ((pos = str.find('&', pos)) != std::string::npos)
    {
        if (pos + 2 >= str.size())
            break;
        
        if (str[pos + 1] == '#')
        {
            const auto isHex = str[pos + 2] == 'x';
            char *end = 0;
            const auto val = strtol(str.c_str() + pos + (isHex ? 3 : 2), &end, isHex ? 16 : 10);
            if (val)
                str.replace(pos, end - (str.c_str() + pos) + 1, 1, static_cast<char>(val));
        }
			else if (knownFixer("nbsp;", " ") ||
                 knownFixer("lt;", "<") ||
                 knownFixer("gt;", ">") ||
                 knownFixer("amp;", "&") ||
                 knownFixer("cent;", "¢") ||
                 knownFixer("pound;", "£") ||
                 knownFixer("yen;", "¥") ||
                 knownFixer("euro;", "€") ||
                 knownFixer("sect;", "§") ||
                 knownFixer("copy;", "©") ||
                 knownFixer("reg;", "®") ||
                 knownFixer("trade;", "™") ||
                 knownFixer("apos;", "'") ||
                 knownFixer("ndash;", "-") ||
                 knownFixer("quot", "\"") ||
                 knownFixer("quot;", "\""))
            ;
        else
            ++pos;
    }
}

} // html namespace ends
} // ay namespace ends 
