#include <barzer_meaning.h>
#include <barzer_universe.h>
#include <ay/ay_logger.h>
extern "C" {
#include <expat.h>
}

namespace barzer {
namespace {

enum {
    TAG_INVALID,
    TAG_SYNONYMS, // top level tag - constains meanings
    TAG_M, // meaning - contains words
    TAG_W, // single word (part of one meaning)
    /// add new tags above this line only
    TAG_MAX
};

#define DECL_TAGHANDLE(X) inline int xmlth_##X( MeaningsXMLParser& parser, int tagId, const char* tag, const char**attr, size_t attr_sz, bool open)

#define IS_PARENT_TAG(X) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X) )
#define IS_PARENT_TAG2(X,Y) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || parser.tagStack.back()== TAG_##Y) )
#define IS_PARENT_TAG3(X,Y,Z) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || \
    parser.tagStack.back()== TAG_##Y || parser.tagStack.back()== TAG_##Z) )

#define ALS_BEGIN    const char** attr_end = attr+ attr_sz;\
	for( const char** a=attr; a< attr_end; a+=2 ) {\
        const char* n = 0[ a ];\
        const char* v = 1[ a ]; \
        switch(n[0]) {

#define ALS_END }}

enum {
    TAGHANDLE_ERROR_OK,
    TAGHANDLE_ERROR_PARENT, // wrong parent 
    TAGHANDLE_ERROR_ENTITY, // misplaced entity 
};

/// wrapper object for expat
struct MeaningsXMLParser_anon {
    XML_ParserStruct*    expat;
    MeaningsXMLParser&   parser;
    MeaningsXMLParser_anon( MeaningsXMLParser& p ) : expat(0),parser(p) {}
    ~MeaningsXMLParser_anon(); 
    MeaningsXMLParser_anon& init();
    void parse( std::istream& fp );
};

/// handlers
DECL_TAGHANDLE(SYNONYMS) {
    parser.d_meaningNameId = 0xffffffff;
    ALS_BEGIN /// attributes
    ALS_END  /// end of attributes
    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(M) {
    if( !IS_PARENT_TAG(SYNONYMS) ) 
        return TAGHANDLE_ERROR_PARENT;

    if( open ) { // tag opening 
        ALS_BEGIN /// attributes
        case 'n': 
			if( !n[1] ) // N - meaning name attribute
				parser.d_meaningNameId = parser.d_gp.internString_internal( v ); 
			break;
		case 'p':
			if (!n[1]) // p - priority attribute
				parser.d_priority = atoi(v);
			break;
        ALS_END  /// end of attributes
    }
    else // tag closing 
	{
        parser.d_meaningNameId = 0xffffffff;
		parser.d_priority = parser.d_defPrio;
	}

    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(W) {
    if( !IS_PARENT_TAG(M) ) 
        return TAGHANDLE_ERROR_PARENT;

    ALS_BEGIN // attributes  loop
        case 'v': 
        if( !n[1] ) { ///  V attribute - value of the word within meaning
            if( parser.d_meaningNameId != 0xffffffff ) { 
                const uint8_t frequency = 0; /// maybe we'll start passing frequency here some day
                const bool addAsUserSpecificString = true; /// this is just done for clarity 

                uint32_t wordId = parser.d_universe->stemAndIntern(v,strlen(v),0);
				parser.d_universe->getBZSpell()->addExtraWordToDictionary(wordId);
                parser.d_universe->meanings().addMeaning( wordId, WordMeaning(parser.d_meaningNameId, parser.d_priority));
            }
        }
            break;
    ALS_END  // end of attributes
    return TAGHANDLE_ERROR_OK;
}

#define SETTAG(X) (handler=((tagId=TAG_##X),xmlth_##X))
inline void tagRouter( MeaningsXMLParser& parser, const char* t, const char** attr, size_t attr_sz, bool open)
{
    char c0= toupper(t[0]),c1=(c0?toupper(t[1]):0),c2=(c1?toupper(t[2]):0),c3=(c2? toupper(t[3]):0);
    typedef int (*TAGHANDLE)( MeaningsXMLParser& , int , const char* , const char**, size_t, bool );
    TAGHANDLE handler = 0;
    int       tagId   =  TAG_INVALID;
    switch( c0 ) {
    case 'M': if( !c1 )     SETTAG(M); break;
    case 'W': if( !c1 )     SETTAG(W); break;

    case 'S': if( c1=='Y' && !strcasecmp(t,"synonyms") ) SETTAG(SYNONYMS); break;
    }
    if( tagId != TAG_INVALID ) {
        if( handler )
            handler(parser,tagId,t,attr,attr_sz,open);

        if( open ) {
            parser.tagStack.push_back( tagId );
        } else if( parser.tagStack.size()  ) {
            parser.tagStack.pop_back();
        } else { // closing tag mismatch
            // maybe we will report an error (however silent non reporting is safer) 
        }
    }
}

} // anonymous namespace 

extern "C" {

// cast to XML_StartElementHandler

static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n;
	const char** atts = (const char**)a; 
	barzer::MeaningsXMLParser_anon *loader =(barzer::MeaningsXMLParser_anon *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( loader->expat );
	if( attrCount <=0 || (attrCount & 1) )
		attrCount = 0; // odd number of attributes is invalid
	loader->parser.tagOpen( name, atts, attrCount );
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	barzer::MeaningsXMLParser_anon *loader =(barzer::MeaningsXMLParser_anon *)ud;
	loader->parser.tagClose(name);
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
    if( !len )
        return;
	const char* s = (const char*)str;
	barzer::MeaningsXMLParser_anon *loader =(barzer::MeaningsXMLParser_anon *)ud;
	if( len>1 || !isspace(*s) ) 
		loader->parser.takeCData( s, len );
}
} // extern "C" ends

namespace {
MeaningsXMLParser_anon& MeaningsXMLParser_anon::init()
{
    if( !expat ) 
        expat= XML_ParserCreate(NULL);
    else 
        XML_ParserReset(expat,0);

    XML_SetUserData(expat,this);
    XML_SetElementHandler(expat, startElement, endElement);
    XML_SetCharacterDataHandler(expat, charDataHandle);

    return *this;
}

MeaningsXMLParser_anon::~MeaningsXMLParser_anon()
{
    if(expat) 
        XML_ParserFree(expat);
}
void  MeaningsXMLParser_anon::parse( std::istream& fp )
{
    if(!expat) {
        AYLOG(ERROR) << "invalid expat instance\n";
    }
	char buf[ 64*1024 ];	
	bool done = false;
    const size_t buf_len = sizeof(buf)-1;
	do {
    	fp.read( buf,buf_len );
    	size_t len = fp.gcount();
    	done = len < buf_len;
    	if (!XML_Parse(expat, buf, len, done)) {
      		fprintf(stderr, "%s at line %d\n", XML_ErrorString(XML_GetErrorCode(expat)), (int)XML_GetCurrentLineNumber(expat));
      		return;
    	}
	} while (!done);
}

} /// anon namespace ends 

/// actual domain specific parsing 

void MeaningsXMLParser::tagOpen( const char* tag, const char** attr, size_t attr_sz )
{
    tagRouter(*this,tag,attr,attr_sz,true);
}

void MeaningsXMLParser::tagClose( const char* tag )
{
    tagRouter(*this,tag,0,0,false);
}

void MeaningsXMLParser::takeCData( const char* dta, size_t dta_len )
{
}

void MeaningsXMLParser::readFromFile( const char* fname )
{
    std::ifstream fp;
    fp.open( fname );
    if( fp.is_open() ) {
        MeaningsXMLParser_anon prs(*this);
        prs.init().parse(fp);
    } else {
        ay::print_absolute_file_path( (std::cerr << "ERROR: MeaningsXMLParser cant open file \"" ), fname ) << "\"\n";
    }
}

} // namespace barzer
