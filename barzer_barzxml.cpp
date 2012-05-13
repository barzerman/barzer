#include <barzer_barz.h>
#include <barzer_barzxml.h>

namespace barzer {

namespace {

enum {
    TAG_BARZ, // top levek barz tag
    TAG_BEAD, // individual bead
    TAG_TOPICS, // topics (parent barz, contains entity)
    TAG_TOKEN, // token ... attribute (opt) stem
    TAG_FLUFF, // 
    TAG_ERC,   
    TAG_LO,   // range low (parent must be range
    TAG_HI,   // range high (parent must be range
    TAG_PUNCT, // parent entlist, bead, erc 
    TAG_ENTITY, // parent entlist, bead, erc 
    TAG_ENTLIST, // entity list - parent barz
    TAG_NUM,    // number. parent - bead/lo/hi attribute t= (int|real)
    TAG_SRCTOK, // parent must be bead
    TAG_RANGE,  // parent is either bead or ERC . attribute order=(ASC|DESC) opt=(NOHI|NOLO|FULLRANGE)
    /// add new tags above this line
    TAG_INVALID, /// invalid tag
    TAG_MAX=TAG_INVALID 
};


/// OPEN handles 
#define DECL_TAGHANDLE(X) inline int xmlth_##X( BarzXMLParser& parser, int tagId, const char* tag, const char**attr)

#define IS_PARENT_TAG(X) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X) )
#define IS_PARENT_TAG2(X,Y) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || parser.tagStack.back()== TAG_##Y) )
#define IS_PARENT_TAG3(X,Y,Z) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || \
    parser.tagStack.back()== TAG_##Y || parser.tagStack.back()== TAG_##Z) )

enum {
    TAGHANDLE_ERROR_OK,
    TAGHANDLE_ERROR_PARENT // wrong parent 
};

DECL_TAGHANDLE(BARZ) { 
    return TAGHANDLE_ERROR_OK;
} 
DECL_TAGHANDLE(BEAD) { 
    if( !IS_PARENT_TAG(BARZ) ) 
        return TAGHANDLE_ERROR_PARENT;

    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(ENTITY) { 
    if( !IS_PARENT_TAG3(ENTLIST,BEAD,ERC) ) 
        return TAGHANDLE_ERROR_PARENT;

    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(ERC) { 
    if( !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(FLUFF) { 
    if( !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(HI) { 
    if( !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;
    
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(LO) { 
    if( !IS_PARENT_TAG(RANGE) ) 
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(PUNCT) { 
    if( !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(NUM) { 
    if( !IS_PARENT_TAG3(BEAD,LO,HI) ) 
        return TAGHANDLE_ERROR_PARENT;
    
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(RANGE) { 
    if( !IS_PARENT_TAG2(BEAD,ERC) )
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(SRCTOK) { 
    if( !IS_PARENT_TAG(BEAD) )
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(TOPICS) { 
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(TOKEN) { 
    BarzelBead& b = parser.barz.appendBlankBead();
    return TAGHANDLE_ERROR_OK;
}

#define SETTAG(X) (handler=((tagId=TAG_##X),xmlth_##X))
inline void tagRouter( BarzXMLParser& parser, const char* t, const char** attr, bool open)
{
    char c0= toupper(t[0]),c1=(c0?toupper(t[1]):0),c2=(c1?toupper(t[2]):0),c3=(c2? toupper(t[3]):0);
    typedef int (*TAGHANDLE)( BarzXMLParser& , int , const char* , const char** );
    TAGHANDLE handler = 0;
    int       tagId   =  TAG_INVALID;
    switch( c0 ) {
    case 'B': 
        if( c1== 'E' && c2 == 'A' )      // BEA ...
            SETTAG(BEAD);
        else if( c1== 'A' && c2 =='R') // BAR ...
            SETTAG(BARZ);
        break;
    case 'E':
        if( c1== 'N' && c2 == 'T' )      // ENT
            SETTAG(BEAD);

        break;
    case 'H':
        break;
    }
    if( tagId != TAG_INVALID ) {
        if( handler ) {
            handler(parser,tagId,t,attr);
        }
        if( open ) {
            parser.tagStack.push_back( tagId );
        } else if( parser.tagStack.size()  ) {
            parser.tagStack.pop_back();
        } else { // closing tag mismatch
            // maybe we will report an error (however silent non reporting is safer) 
        }
    }
}



} // end of anonymous namespace 

void BarzXMLParser::takeTag( const char* tag, const char** attr, bool open)
{
    tagRouter(*this,tag,attr,open);
}

void BarzXMLParser::takeCData( const char* dta, const char* dta_len )
{
    if( !tagStack.size() || barz.isListEmpty() ) {
        return;
    }
    BarzelBead& bead = barz.getLastBead();
    BarzerLiteral* ltrl = bead.get<BarzerLiteral>( );
    int curTag = tagStack.back();
    switch( curTag ) {
    case TAG_TOKEN:
        if( ltrl ) {
        }
        break;
    case TAG_FLUFF:
        if( ltrl ) {
        }
        break;
    case TAG_PUNCT:
        if( ltrl ) {
        }
        break;
    case TAG_NUM:
        {
            BarzerNumber* num = bead.get<BarzerNumber>();
            if( num ) {
            }
        }
        break;
    default:
        // error can be reported here
        break;
    }
}

} // namespace barzer
