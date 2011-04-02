#include <barzer_el_xml.h>

extern "C" {
#include <expat.h>

// cast to XML_StartElementHandler
static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n; 
	const char** atts = (const char**)a; 
	barzer::BELParserXML *loader =(barzer::BELParserXML *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( loader->parser );
	if( attrCount <=0 || (attrCount & 1) )
		attrCount = 0; // odd number of attributes is invalid
	loader->startElement( name, atts, attrCount );
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	barzer::BELParserXML *loader =(barzer::BELParserXML *)ud;
	loader->endElement(name);
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
	const char* s = (const char*)str;
	barzer::BELParserXML *loader =(barzer::BELParserXML *)ud;
	if( len>1 || !isspace(*s) ) 
		loader->getElementText( s, len );
}
} // extern "C" ends

namespace barzer {

// attr_sz number of attribute pairs
void BELParserXML::startElement( const char* tag, const char_cp * attr, size_t attr_sz )
{
}

void BELParserXML::endElement( const char* tag )
{
}
void BELParserXML::getElementText( const char* txt, int len )
{
}
BELParserXML::~BELParserXML()
{
	if( parser ) 
		XML_ParserFree(parser);
}

int BELParserXML::parse( std::istream& fp )
{
	/// initialize parser  if needed
	if( !parser ) {
		parser = XML_ParserCreate(NULL);
		if( !parser ) {
			std::cerr << "BELParserXML couldnt create xml parser\n";
			return 0;
		}
		XML_SetUserData(parser,this);
		XML_SetElementHandler(parser, 
			::startElement, ::endElement);
		XML_SetCharacterDataHandler(parser, charDataHandle);
	}
	
	char buf[ 128*1024 ];	
	bool done = false;
	do {
    	fp.read( buf,sizeof(buf) );
    	size_t len = fp.gcount();
    	done = len < sizeof(buf);
    	if (!XML_Parse(parser, buf, len, done)) {
      		fprintf(stderr,
	      		"%s at line %d\n",
	      		XML_ErrorString(XML_GetErrorCode(parser)),
	      		(int)XML_GetCurrentLineNumber(parser));
      		return 1;
    	}
	} while (!done);
	return 0;
}

#define CHECK_1CW(N)  if( !s[1] ) return N;
#define CHECK_2CW(c,N)  if( c[0] == s[1] && !s[2] ) return N;
#define CHECK_3CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && !s[3] ) return N;
#define CHECK_4CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && c[2] == s[3] && !s[4] ) return N;

int BELParserXML::getTag( const char* s ) const
{
	switch( *s ) {
	case 'a':
	CHECK_3CW("ny",TAG_ANY) // <any>
		break;
	case 'd':
	CHECK_4CW("ate", TAG_DATE ) // <date> 
		break;
	case 'f':
	CHECK_4CW("unc", TAG_FUNC ) // <func>
		break;
	case 'l':
	CHECK_4CW("list", TAG_LIST ) // <list>
		break;
	case 'n':
	CHECK_1CW(TAG_N) // <n>
		break;
	case 'o':
	CHECK_3CW("pt", TAG_OPT ) // <opt>
		break;
	case 'p':
	CHECK_1CW(TAG_P) // <p> 
	CHECK_3CW("at",TAG_PATTERN ) // <pat>
	CHECK_4CW("erm",TAG_PERM ) // <perm>
		break;
	case 'r': 
		break;
	case 's':
	CHECK_4CW("tmt",TAG_STATEMENT )  // <stmt>
		break;
	case 't':
	CHECK_1CW(TAG_T) // <t>
	CHECK_2CW("g",TAG_TG) // <tg>
	CHECK_4CW("ail",TAG_TAIL) // <tail>
	CHECK_4CW("drv",TAG_TDRV) // <tdrv>
	CHECK_4CW("ime",TAG_TIME) // <time>
	CHECK_4CW("ran",TAG_TRANSLATION) // <tran>
		break;
	case 'v':
	CHECK_3CW("ar",TAG_VAR) // <var>
		break;
	case 'w':
	CHECK_1CW(TAG_W) // <w>
	CHECK_4CW("cls",TAG_WCLS) // <wcls>
	default:
		return TAG_UNDEFINED;
	} // switch ends
	
	return TAG_UNDEFINED;
}

} // barzer namespace ends
