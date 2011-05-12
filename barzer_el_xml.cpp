#include <barzer_el_xml.h>
#include <ay/ay_debug.h>
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
		// in fact statement can't even have an undefined parent anymore.
		// should probably rewrite it /pltr
		return ( tag == TAG_STATEMENT || tag == TAG_STMSET);
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
	case TAG_DATETIME:
	case TAG_ENTITY:
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
		std::cerr << "invalid BEL xml(tag: " << tag <<") in statement " << statementCount << "\n";
		return;
	}	
	elementHandleRouter( tid, attr,attr_sz, false );
}

void BELParserXML::elementHandleRouter( int tid, const char_cp * attr, size_t attr_sz, bool close )
{
	//AYDEBUG(tid);
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
	CASE_TAG(DATETIME)
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

	//if( close )
	//	statement.popNode();

}

void BELParserXML::taghandle_STATEMENT( const char_cp * attr, size_t attr_sz, bool close )
{
	if( close ) { /// statement is ready to be sent to the reader for adding to the trie
		if( statement.hasStatement() ) {
			reader->addStatement( statement.stmt );
		}
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
/// tag text visitors
namespace {

struct BTND_text_visitor_base  : public boost::static_visitor<> {
	BELParserXML& d_parser;
	const char* d_str;
	int d_len;
	
	BTND_text_visitor_base( BELParserXML& parser, const char* s, int len ) : 
		d_parser(parser),
		d_str(s), 
		d_len(len) 
	{}
};

// rewrite visitor template specifications 
struct BTND_Rewrite_Text_visitor : public BTND_text_visitor_base {
	BTND_Rewrite_Text_visitor( BELParserXML& parser, const char* s, int len ) :
		BTND_text_visitor_base( parser, s, len ) 
	{}

	template <typename T>
	void operator()(T& t) const { }
};
template <> void BTND_Rewrite_Text_visitor::operator()<BTND_Rewrite_Literal>(BTND_Rewrite_Literal& t)   const
	{ 
		uint32_t i = d_parser.internTmpText(d_str, d_len); 
		if( t.isCompound() ) {
			// warning compounded tokens unsupported yet
		} else {
			t.setId( i );
		}
	}
template <> void BTND_Rewrite_Text_visitor::operator()<BTND_Rewrite_Number>(BTND_Rewrite_Number& t)   const
	{ 
		const char* s = d_parser.setTmpText( d_str, d_len );
		if( strchr( s, '.' ) ) 
			t.set( atof(s) );
		else
			t.set( atoi(s) );
	}

// pattern visitor  template specifications 
struct BTND_Pattern_Text_visitor : public BTND_text_visitor_base {
	BTND_Pattern_Text_visitor( BELParserXML& parser, const char* s, int len ) :
		BTND_text_visitor_base( parser, s, len ) 
	{}
	template <typename T>
	void operator() (T& t) const {}
};

template <> void BTND_Pattern_Text_visitor::operator()<BTND_Pattern_Token>  (BTND_Pattern_Token& t)  const
{ t.stringId = d_parser.internTmpText(  d_str, d_len  ); }
template <> void BTND_Pattern_Text_visitor::operator()<BTND_Pattern_Punct> (BTND_Pattern_Punct& t) const
{ 
	// may want to do something fancy for chinese punctuation (whatever that is)
	const char* str_end = d_str + d_len;
	for( const char* s = d_str; s< str_end; ++s ) {
		if( !isspace(*s) ) {
			t.setChar( *s );
			return;
		}
	}
	t.setChar(0);
}
// pattern visitor  template specifications 

// general visitor 
struct BTND_text_visitor : public BTND_text_visitor_base {
	BTND_text_visitor( BELParserXML& parser, const char* s, int len ) :
		BTND_text_visitor_base( parser, s, len ) 
	{}
	void operator()( BTND_PatternData& pat ) const
		{ boost::apply_visitor( BTND_Pattern_Text_visitor(d_parser,d_str,d_len), pat ) ; }
	void operator()( BTND_None& ) const {}
	void operator()( BTND_StructData& ) const {}
	void operator()( BTND_RewriteData& rwr)  const
		{ boost::apply_visitor( BTND_Rewrite_Text_visitor(d_parser,d_str,d_len), rwr ) ; }
};

} // end of anon namespace 

void BELParserXML::taghandle_T( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	bool isStop = false;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 't':  // type t=
			switch( v[0] ) {
			// stop is the same as fluff
			case 'f': // fluff (t="f" or t="fluff")
			case 's': // stop (t="s" or t="stop") 
				isStop = true;
				break;
			}
			break;
		}
	}
	BTND_PatternData dta;
	if( isStop ) 
		dta = BTND_Pattern_StopToken();
	else
		dta = BTND_Pattern_Token();
	statement.pushNode( dta );
}

void BELParserXML::taghandle_TG( const char_cp * attr, size_t attr_sz , bool close)
{
}
void BELParserXML::taghandle_P( const char_cp * attr, size_t attr_sz , bool close)
{
	if (close) {
		statement.popNode();
		return;
	} 
	statement.pushNode( BTND_PatternData( BTND_Pattern_Punct()));
}
void BELParserXML::taghandle_SPC( const char_cp * attr, size_t attr_sz , bool close)
{
}
void BELParserXML::taghandle_N( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
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
	statement.pushNode( BTND_PatternData( pat));
}
void BELParserXML::taghandle_ENTITY( const char_cp * attr, size_t attr_sz , bool close) 
{ 
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Pattern_Entity pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'c': // class - c="1"
			pat.setEntityClass( atoi(v) ); 
			break;
		case 's': // subclass - s="1"
			pat.setEntityClass( atoi(v) ); 
			break;
		case 't': // id token - t="ABCD011"
			pat.setTokenId( internString(v) );
			break;
		}
	}
	statement.pushNode( BTND_PatternData( pat));
}
void BELParserXML::taghandle_DATETIME( const char_cp * attr, size_t attr_sz , bool close) 
{ 
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Pattern_DateTime pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'd': 
			if( n[1] == 'l' ) { /// dl="YYYYMMDD" date low 
				pat.setLoDate( atoi(v) );
			} else if( n[1] == 'h' ) { // dh="YYYYMMDD" date hi
				pat.setHiDate( atoi(v) );
			}
			break;
		case 'f':  // f="y" - future
			pat.setFuture();
			break;
		case 'p':  // f="y" - future
			pat.setPast();
			break;
		case 't': 
			if( n[1] == 'l' ) { /// tl="hhmmss" time low 
				pat.setLoTime( atoi(v) );
			} else if( n[1] == 'h' ) { // th="hhmmss" time hi
				pat.setHiTime( atoi(v) );
			}
			break;
		}
	}
	statement.pushNode( BTND_PatternData( pat));
}
void BELParserXML::taghandle_DATE( const char_cp * attr, size_t attr_sz , bool close) 
{ 
	if( close ) {
		statement.popNode();
		return;
	}
	/// we can actually replace future and past with ranges, where today is computed for the
	/// current instance of the engine in case there are problems 
	BTND_Pattern_Date pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		/// note: value of the attribute is ignored for everything except for hi and lo 
		/// all other attributes are boolean
		switch( *n ) {
		// any <date a="y"/> - thi is a default option <date/> accomplishes just that
		case 'a': pat.setPast(); break;
		// future <date f="y"/> 
		case 'f': pat.setFuture(); break;
		// high same as low 'l'
		case 'h': pat.setLo(atoi(v)); break;
		// low date <date l="20100908" h="20110101"/> 
		case 'l': pat.setLo(atoi(v)); break;
		// past <date p="yes"/> or <date past="y"/>
		case 'p': pat.setPast(); break;
		}
	}	
	statement.pushNode( BTND_PatternData( pat));
}

void BELParserXML::taghandle_TIME( const char_cp * attr, size_t attr_sz , bool close) 
{
}

void BELParserXML::taghandle_RX( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_TDRV( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_WCLS( const char_cp * attr, size_t attr_sz , bool close)
{}
void BELParserXML::taghandle_W( const char_cp * attr, size_t attr_sz , bool close)
{
}

void BELParserXML::processAttrForStructTag( BTND_StructData& dta, const char_cp * attr, size_t attr_sz )
{
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'v': 
			dta.setVarId( internString(v) );
			return;
		}
	}
}
void BELParserXML::taghandle_LIST( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_LIST);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
void BELParserXML::taghandle_ANY( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_ANY);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
void BELParserXML::taghandle_OPT( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_OPT);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
void BELParserXML::taghandle_PERM( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_PERM);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
void BELParserXML::taghandle_TAIL( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_TAIL);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode(node);
}
/// rewrite tags 
void BELParserXML::taghandle_LITERAL( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Literal literal;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 't': // type
			switch( v[0] ) {
			case 'f': // t="fluff" ~ stop token
			case 's': // t="stop" ~ stop token
				literal.setStop();
				break;
			case 'c': // t="comp" ~ compounded word
				literal.setCompound();
				break;
			}
			break;
		}
	}
	statement.pushNode( BTND_RewriteData(literal) );
}

void BELParserXML::taghandle_RNUMBER( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Number num;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'v': {
			if( strchr( v, '.' ) ) {
				num.set( atof( v ) );
			} else {
				num.set( atoi( v ) );
			}
		}
			break;
		}
	}
	statement.pushNode( 
		BTND_RewriteData(
			num
		)
	);
}
void BELParserXML::taghandle_VAR( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Variable var;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value

		switch( n[0] ) {
		case 'g': { // <var gn="1"/> gap number - if pattern is a * b c * d gn[1] is a, gn[2] = (b c), gn[3] = 3 
			int num  = atoi(v);
			if( num > 0 ) 
				var.setWildcardGapNumber( num );
		}
			 break;
		case 'n': { // <var name=""/> - named variable
			/// intern
			var.setVarId( internVariable(v) );
		}
			break;
		case 'p': { // <var pn="1"/> - pattern element number - if pattern is "a * b * c" then pn[1] is a, pn[2] is the first * etc
					// pn[0] is the whole sub chain matching this pattern 
			int num  = atoi(v);
			if( num > 0 ) 
				var.setPatternElemNumber( num );
			else {
				AYLOG(ERROR) << "invalid pattern element number " << v << std::endl;
			}
		}
			break;
		case 'w': { // <var w="1"/> - wildcard number like $1 
			int num  = atoi(v);
			if( num > 0 ) 
				var.setWildcardNumber( num );
			else {
				AYLOG(ERROR) << "invalid wildcard number " << v << std::endl;
			}
		}
			break;

		}
	}
	statement.pushNode( BTND_RewriteData( var));
}
void BELParserXML::taghandle_FUNC( const char_cp * attr, size_t attr_sz , bool close)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Function f;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		if( !strcmp(n, "name") ) {
		 f.setNameId( internString(v) );
		}
	}
	statement.pushNode( BTND_RewriteData( f));
}

// not sure if it's even needed here /pltr
void taghandle_STMSET( const char_cp * attr, size_t attr_sz , bool close=false) {}


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
	BELParseTreeNode* node = statement.getCurrentNode();
	if( node ) 	{
		boost::apply_visitor( BTND_text_visitor(*this,txt,len), node->getVar() ) ; 
	}
	return;

	/*
	int tid = tagStack.top();

#define CASE_TAG(x) case TAG_##x: taghandle_##x##_text( txt, len ); break;
	switch( tid ) {
		CASE_TAG(T)
		CASE_TAG(LITERAL)
		CASE_TAG(RNUMBER)
	}
#undef CASE_TAG
	*/
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
// needed for stmset
#define CHECK_6CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && c[2] == s[3] && c[3] == s[4] && c[4] == s[5] && !s[6] ) return N;

int BELParserXML::getTag( const char* s ) const
{
	switch( *s ) {
	case 'a':
	CHECK_3CW("ny",TAG_ANY) // <any>
		break;
	case 'd':
	CHECK_4CW("ate", TAG_DATE ) // <date> 
	CHECK_4CW("tim", TAG_DATETIME ) // <dtim> 
		break;
	case 'e':
	CHECK_3CW("nt",TAG_ENTITY) // <ent>
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
	CHECK_6CW("tmset",TAG_STMSET )  // <stmset>
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

void BELParserXML::CurStatementData::clear()
{
	bits.clear();
	state = STATE_BLANK;
	stmt.translation.clear();
	stmt.pattern.clear();
	if ( !nodeStack.empty() ) {
		AYDEBUG(nodeStack.size());
		while (!nodeStack.empty())
			nodeStack.pop();
	}
}

void BELParserXML::CurStatementData::popNode()
{
	//AYTRACE("popNode");
	nodeStack.pop();
}

} // barzer namespace ends
