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

bool BELParserXML::isValidTag( int tag, int parent ) const
{
	switch( parent ) {
	case TAG_UNDEFINED:
		return ( tag == TAG_STATEMENT );
	case TAG_STATEMENT:
		if( tag == TAG_PATTERN ) {
			if( !statement.pattern )
				return true;
		} else if( tag == TAG_TRANSLATION ) {
			if( !statement.translation )
				return true;
		}
		return false;
	case TAG_PATTERN:
	switch( tag ) {
	TAG_T:
	TAG_TG:
	TAG_P:
	TAG_SPC:
	TAG_N:
	TAG_RX:
	TAG_TDRV:
	TAG_WCLS:
	TAG_W:
	TAG_DATE:
	TAG_TIME:
	TAG_LIST:
	TAG_ANY:
	TAG_OPT:
	TAG_PERM:
	TAG_TAIL:
		return true;
	default:
		return false;
	}
		break;

	case TAG_TRANSLATION:
		return true;
	default:
		return false;
	}
	// fall through - should reach here 	
	return false;
}

// attr_sz number of attribute pairs
void BELParserXML::startElement( const char* tag, const char_cp * attr, size_t attr_sz )
{
	int tid = getTag( tag );
	int parentTag = ( tagStack.empty() ? TAG_UNDEFINED:  tagStack.top() );
	
	if( tid == TAG_STATEMENT ) {
		++statementCount;
	}
	tagStack.push( tid );
	if( !isValidTag( tid, parentTag )  ) {
		std::cerr << "invalid BEL xml in statement " << statementCount << "\n";
		return;
	}	
	elementHandleRouter( tid, attr,attr_sz, false );
}

void BELParserXML::elementHandleRouter( int tid, const char_cp * attr, size_t attr_sz, bool close )
{
#define CASE_TAG(x) case TAG_##x: taghandle_##x(attr,attr_sz,close); return;
	if( close ) 
		statement.popNode();

	switch( tid ) {
	CASE_TAG(UNDEFINED)
	CASE_TAG(STATEMENT)
	CASE_TAG(PATTERN)
	CASE_TAG(TRANSLATION)
	CASE_TAG(T)
	CASE_TAG(TG)
	CASE_TAG(P)
	CASE_TAG(SPC)
	CASE_TAG(N)
	CASE_TAG(RX)
	CASE_TAG(TDRV)
	CASE_TAG(WCLS)
	CASE_TAG(W)
	CASE_TAG(DATE)
	CASE_TAG(TIME)
	CASE_TAG(LIST)
	CASE_TAG(ANY)
	CASE_TAG(OPT)
	CASE_TAG(PERM)
	CASE_TAG(TAIL)
	}
#undef CASE_TAG
}

void BELParserXML::taghandle_STATEMENT( const char_cp * attr, size_t attr_sz, bool close )
{
	if( close ) { /// tag is being closed
		return;
	}
	if( statement.hasStatement() ) { // bad - means we have statement tag nested in another statement
		std::cerr << "statement nested in statement " << statementCount << "\n";
		return;
	} 
	statement.setStatement();
}
void BELParserXML::taghandle_UNDEFINED( const char_cp * attr, size_t attr_sz, bool close )
{}

void BELParserXML::taghandle_PATTERN( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) { /// tag is being closed
		return;
	}
	if( statement.hasPattern() ) { 
		std::cerr << "duplicate pattern tag in statement " << statementCount << "\n";
		return;
	}
	
}
void BELParserXML::taghandle_TRANSLATION( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) { /// tag is being closed
		statement.popNode();
		return;
	}
	if( statement.hasTranslation() ) { 
		std::cerr << "duplicate translation tag in statement " << statementCount << "\n";
		return;
	}
	
}
void BELParserXML::taghandle_T( const char_cp * attr, size_t attr_sz , bool close)
{
}
void BELParserXML::taghandle_TG( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_P( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_SPC( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_N( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_RX( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_TDRV( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_WCLS( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_W( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_DATE( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_TIME( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_LIST( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_ANY( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_OPT( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_PERM( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_TAIL( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_TRANSLATION( const char_cp * attr, size_t attr_sz , bool close)
{}

void BELParserXML::endElement( const char* tag )
{
	int tid = getTag( tag );
	elementHandleRouter(tid,0,0,true);
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
