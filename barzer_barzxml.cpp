#include <barzer_barz.h>
#include <barzer_barzxml.h>

namespace barzer {

namespace {

    enum {
        TAG_BARZ, // top levek barz tag
        TAG_BEAD, // individual bead
        TAG_TOKEN, // token ... attribute (opt) stem
        TAG_FLUFF, // 
        TAG_ERC,   
        TAG_LO,   // range low (parent must be range
        TAG_HI,   // range high (parent must be range
        TAG_PUNCT, // parent entlist, bead, erc 
        TAG_ENTITY, // parent entlist, bead, erc 
        TAG_NUM,    // number. parent - bead/lo/hi attribute t= (int|real)
        TAG_SRCTOK, // parent must be bead
        TAG_RANGE  // parent is either bead or ERC . attribute order=(ASC|DESC) opt=(NOHI|NOLO|FULLRANGE)
    };

} // end of anonymous namespace 

void BarzXMLParser::takeTag( const char* tag, const char** attr, bool open)
{
}

void BarzXMLParser::takeCData( const char* dta, const char* dta_len )
{
}

} // namespace barzer
