/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 

////// THIS IS A TEMPLATE forcustomized XML PARSER 
///// clone it into a new file and fill out the TAGS  (search for XXX) 
#include <iostream>
#include <vector>
#include <string>
#include <expat.h>
#include <ay_logger.h>

/// KEEP IT IN ANONYMOUS NAMESPACE
namespace {

struct ay_xml_parser {
    XML_ParserStruct* expat;
	std::ostream& d_os;

    // current stack of tags - see .cpp file for tag codes 
    std::vector< int > tagStack;

    void takeTag( const char* tag, const char** attr, size_t attr_sz, bool open=true );
    void takeCData( const char* dta, size_t dta_len );
    
    bool isCurTag( int tid ) const
        { return ( tagStack.back() == tid ); }
    bool isParentTag( int tid ) const
        { return ( tagStack.size() > 1 && (*(tagStack.rbegin()+1)) == tid ); }

	ay_xml_parser& init();
	void parse( std::istream& fp );

	~ay_xml_parser();
	ay_xml_parser( std::ostream& os ) :
	    d_os(os)
	{ }

};

enum {
    TAG_XXX,
    /// add new tags above this line
    TAG_INVALID, /// invalid tag
    TAG_MAX=TAG_INVALID 
};


/// OPEN handles 
#define DECL_TAGHANDLE(X) inline int xmlth_##X( ay_xml_parser& parser, int tagId, const char* tag, const char**attr, size_t attr_sz, bool open)

/// IS_PARENT_TAG(X) (X1,X2) (X1,X2,X3) - is parent tag one of the ... 
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

#define UNIVERSE parser.universe
#define GLOBALPOOLS parser.universe.getGlobalPools()

enum {
    TAGHANDLE_ERROR_OK,
    TAGHANDLE_ERROR_PARENT, // wrong parent 
    TAGHANDLE_ERROR_ENTITY, // misplaced entity 
};

////////// TAG HANDLES  
DECL_TAGHANDLE(XXX) { 
    /// IS_PARENT_TAG3(X) (X,Y) or (X,Y,Z)
    if( open ) {
        /// loops over all atributes
	    ALS_BEGIN 
            case 'x': printf( "%s\n", v ); 
	    ALS_END
    } else { // closing BARZ - time to process
    }
    return TAGHANDLE_ERROR_OK;
} 
////////// DEFINE SPECIFIC TAG HANDLES ABOVE (SEE DECL_TAGHANDLE(XXX) )

#define SETTAG(X) (handler=((tagId=TAG_##X),xmlth_##X))
inline void tagRouter( ay_xml_parser& parser, const char* t, const char** attr, size_t attr_sz, bool open)
{
    char c0= toupper(t[0]),c1=(c0?toupper(t[1]):0),c2=(c1?toupper(t[2]):0),c3=(c2? toupper(t[3]):0);
    typedef int (*TAGHANDLE)( ay_xml_parser& , int , const char* , const char**, size_t, bool );
    TAGHANDLE handler = 0;
    int       tagId   =  TAG_INVALID;
    switch( c0 ) {
    case 'x': SETTAG(XXX); break;
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

void ay_xml_parser::takeTag( const char* tag, const char** attr, size_t attr_sz, bool open)
    { tagRouter(*this,tag,attr,attr_sz,open); }

void ay_xml_parser::takeCData( const char* dta, size_t dta_len )
{ 
    /// if it's in your power enforce no CData in the schema
}

extern "C" {

// cast to XML_StartElementHandler
static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n;
	const char** atts = (const char**)a; 
	ay_xml_parser *parser =(ay_xml_parser *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( parser->expat );
	if( attrCount <=0 || (attrCount & 1) )
		attrCount = 0; // odd number of attributes is invalid
	parser->takeTag( name, atts, attrCount );
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	ay_xml_parser *parser =(ay_xml_parser *)ud;
	parser->takeTag(name, 0,0,false);
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
    if( !len )
        return;
	const char* s = (const char*)str;
	ay_xml_parser *parser =(ay_xml_parser *)ud;
	if( len>1 || !isspace(*s) ) 
		parser->takeCData( s, len );
}
} // extern "C" ends

ay_xml_parser& ay_xml_parser::init()
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

ay_xml_parser::~ay_xml_parser()
{
    if(expat) 
        XML_ParserFree(expat);
}
void  ay_xml_parser::parse( std::istream& fp )
{
    if(!expat) {
        AYLOG(ERROR) << "invalid expat instance\n";
    }
	char buf[ 1024*1024 ];	
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

} // your namespace ends

