#include <barzer_el_xml.h>
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
			if( !statement.hasPattern() )
				return true;
		} else if( tag == TAG_TRANSLATION ) {
			if( !statement.hasTranslation() )
				return true;
		}
		return false;
	case TAG_PATTERN:
	switch( tag ) {
	case TAG_T:
	case TAG_TG:
	case TAG_P:
	case TAG_SPC:
	case TAG_N:
	case TAG_RX:
	case TAG_TDRV:
	case TAG_WCLS:
	case TAG_W:
	case TAG_DATE:
	case TAG_TIME:
	case TAG_LIST:
	case TAG_ANY:
	case TAG_OPT:
	case TAG_PERM:
	case TAG_TAIL:
		return true;
	default:
		return false;
	}
		break;

	case TAG_TRANSLATION:
		return true;
	default:
		return true;
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

	CASE_TAG(LITERAL)
	CASE_TAG(RNUMBER)
	CASE_TAG(VAR)
	CASE_TAG(FUNC)
	}
#undef CASE_TAG
	if( close ) 
		statement.popNode();
}

void BELParserXML::taghandle_STATEMENT( const char_cp * attr, size_t attr_sz, bool close )
{
	if( close ) { /// statement is ready to be sent to the reader for adding to the trie
		if( statement.hasStatement() ) 
			reader->addStatement( statement.stmt );
		statement.clear();
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
		statement.setBlank();
		return;
	}
	if( statement.hasPattern() ) { 
		std::cerr << "duplicate pattern tag in statement " << statementCount << "\n";
		return;
	}
	statement.setPattern();
}
void BELParserXML::taghandle_TRANSLATION( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) { /// tag is being closed
		statement.setBlank();
		return;
	}
	if( statement.hasTranslation() ) { 
		std::cerr << "duplicate translation tag in statement " << statementCount << "\n";
		return;
	}
	statement.setTranslation();
}

void BELParserXML::taghandle_T_text( const char* s, int len )
{
	BELParseTreeNode* node = statement.getCurrentNode();
	if( node ) {
		BTND_Pattern_Token* t = 0;
		node->getNodeDataPtr(t);
		if( t ) {
			d_tmpText.assign( s, len );
			ay::UniqueCharPool::StrId sid = strPool->internIt( d_tmpText.c_str() );
			t->stringId = sid;
		}
	}
}
void BELParserXML::taghandle_T( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( 
		BTND_PatternData(
			BTND_Pattern_Token() 
		)
	);

}
void BELParserXML::taghandle_TG( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_P( const char_cp * attr, size_t attr_sz , bool close)
{
}
void BELParserXML::taghandle_SPC( const char_cp * attr, size_t attr_sz , bool close)
{
}
void BELParserXML::taghandle_N( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;

	bool isReal = false, isRange = false;
	float flo=0., fhi = 0.;
	int   ilo = 0, ihi = 0;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'h':
			if( !isRange ) isRange = true;

			if( !isReal ) 
				isReal = strchr(v, '.');
			if( isReal ) {
				fhi = atof( v );
				ihi = (int) fhi;
			} else {
				ihi = atoi( v );
				fhi = (float) fhi;
			}
			break;
		case 'l':
			if( !isRange ) isRange = true;
			if( !isReal ) 
				isReal = strchr(v, '.');
			if( isReal ) {
				flo = atof( v );
				ilo = (int) fhi;
			} else {
				ilo = atoi( v );
				flo = (float) fhi;
			}
			break;
		case 'r':
			if( !isReal ) isReal = true;
			break;
		}
	}
	BTND_Pattern_Number pat; 
	
	if( isRange ) {
		if( isReal ) 
			pat.setRealRange( flo, fhi );
		else 
			pat.setIntRange( ilo, ihi );
	} else {
		if( isReal ) 
			pat.setAnyReal();
		else
			pat.setAnyInt();
	}
	statement.pushNode( 
		BTND_PatternData(
			pat
		)
	);
}
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
{
	if( close ) return;
	statement.pushNode( BTND_StructData( BTND_StructData::T_LIST));
}
void BELParserXML::taghandle_ANY( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( BTND_StructData( BTND_StructData::T_ANY));
}
void BELParserXML::taghandle_OPT( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( BTND_StructData( BTND_StructData::T_OPT));
}
void BELParserXML::taghandle_PERM( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( BTND_StructData( BTND_StructData::T_PERM));
}
void BELParserXML::taghandle_TAIL( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( BTND_StructData( BTND_StructData::T_TAIL));
}
/// rewrite tags 
void BELParserXML::taghandle_LITERAL_text( const char* s, int len )
{
	BELParseTreeNode* node = statement.getCurrentNode();
	if( node ) {
		BTND_Rewrite_Literal* t = 0;
		node->getNodeDataPtr(t);
		if( t ) {
			d_tmpText.assign( s, len );
			ay::UniqueCharPool::StrId sid = strPool->internIt( d_tmpText.c_str() );
			t->setString( sid );
		}
	}
}
void BELParserXML::taghandle_LITERAL( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( 
		BTND_RewriteData(
			BTND_Rewrite_Literal() 
		)
	);
}
void BELParserXML::taghandle_RNUMBER_text( const char*s, int len )
{
	BELParseTreeNode* node = statement.getCurrentNode();
	if( node ) {
		BTND_Rewrite_Number* t = 0;
		node->getNodeDataPtr(t);
		if( t && len && s[0] ) {
			d_tmpText.assign( s, len );
			if( strchr( d_tmpText.c_str(), '.' ) ) {
				t->set( atof( d_tmpText.c_str() ) );
			} else {
				t->set( atoi( d_tmpText.c_str() ) );
			}
		}
	}
}
void BELParserXML::taghandle_RNUMBER( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
	}
	statement.pushNode( 
		BTND_RewriteData(
			BTND_Rewrite_Number(0) 
		)
	);
}

void BELParserXML::taghandle_VAR( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( 
		BTND_RewriteData(
			BTND_Rewrite_Variable() 
		)
	);
}
void BELParserXML::taghandle_FUNC( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) return;
	statement.pushNode( 
		BTND_RewriteData(
			BTND_Rewrite_Function() 
		)
	);
}


void BELParserXML::endElement( const char* tag )
{
	int tid = getTag( tag );
	elementHandleRouter(tid,0,0,true);
	tagStack.pop();
}

void BELParserXML::getElementText( const char* txt, int len )
{
	if( tagStack.empty() )
		return; // this should never happen 
	int tid = tagStack.top();
#define CASE_TAG(x) case TAG_##x: taghandle_##x##_text( txt, len ); break;
	switch( tid ) {
		CASE_TAG(T)
		CASE_TAG(LITERAL)
		CASE_TAG(RNUMBER)
	}
#undef CASE_TAG
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
	CHECK_4CW("ist", TAG_LIST ) // <list>
	CHECK_4CW("trl", TAG_LITERAL ) // <ltrl>
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
	CHECK_2CW("n",TAG_RNUMBER) // <rn>
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
