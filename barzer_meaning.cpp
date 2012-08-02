#include <barzer_meaning.h>
#include <ay/ay_logger.h>
extern "C" {
#include <expat.h>
}

namespace barzer {
namespace {

enum {
    TAG_INVALID,
    TAG_M,
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
        switch(n[1]) {

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
DECL_TAGHANDLE(M) {
    ALS_BEGIN
    ALS_END
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
    case 'M': 
        if( !c1 ) 
            SETTAG(M);
        break;
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
	const char c = tag[0];
	switch (c)
	{
	case 'm':
		for (size_t i = 0; i < attr_sz; i += 2)
			if (attr[i][0] == 'n')
			{
				m_meaningName.assign(attr[i + 1]);
				break;
			}
		break;
	case 'w':
		for (size_t i = 0; i < attr_sz; i += 2)
			if (attr[i][0] == 'v')
			{
				m_curWords.push_back(std::string(attr[i + 1]));
				break;
			}
		break;
	}
	
}

void MeaningsXMLParser::tagClose( const char* tag )
{
	if (tag[0] == 'm')
	{
		RawMeaning m =
		{
			m_meaningName,
			m_curWords,
			0 // todo
		};
		m_parsedMeanings.push_back(m);
		m_meaningName.clear();
		m_curWords.clear();
	}
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

void MeaningsXMLParser::clear()
{
	m_meaningName.clear();
	m_curWords.clear();
	m_parsedMeanings.clear();
}

} // namespace barzer
