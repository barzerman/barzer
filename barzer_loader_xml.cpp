#include <barzer_dtaindex.h>
#include <barzer_loader_xml.h>

extern "C" {
#include <expat.h>

// cast to XML_StartElementHandler
static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n; 
	const char** atts = (const char**)a; 
	barzer::EntityLoader_XML *loader =(barzer::EntityLoader_XML *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( loader->parser );
	if( attrCount <=0 || (attrCount & 1) )
		attrCount = 0; // odd number of attributes is invalid
	loader->startElement( name, atts, attrCount );
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	barzer::EntityLoader_XML *loader =(barzer::EntityLoader_XML *)ud;
	loader->endElement(name);
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
	const char* s = (const char*)str;
	barzer::EntityLoader_XML *loader =(barzer::EntityLoader_XML *)ud;
	if( len>1 || !isspace(*s) ) 
		loader->getElementText( s, len );
}
} // extern "C" ends


namespace barzer {

EntityLoader_XML::Tag_t EntityLoader_XML::getTag( const char* s ) const
{
	switch(s[0]) {
	case 'e':
		if( !s[1] )  { // <e>  entity
			return TAG_ENTITY;
		} else if( !strcmp( &(s[1]), "ntlist" ) ) { // <entlist>
			return TAG_ENTLIST;
		}
		break;
	case 'n':
		if( !s[1] ) { // <n> ~ name
			return TAG_NAME;
		}
		break;
	case 't':
		if( !s[1] ) { // <t>
			return TAG_TOKEN;
		}
		break;
	}
	return TAG_NULL;
}

void EntityLoader_XML::handle_entity_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz )
{
	StoredEntityUniqId euid;
	/// forming uniqId based on the attributes
	StoredToken* idTok_p = 0;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'c':
			if( !n[1] ) euid.eclass.ec = atoi(v);
			break;
		case 's':
			if( !n[1] ) euid.eclass.subclass = atoi(v);
			break;
		case 'i':
			if( !n[1] ) {
				// need to populate euid.tokId 
				// first must resolve token
				bool newAdded = true;
				StoredToken& tok = dtaIdx->tokPool.addSingleTok( newAdded, v );
				idTok_p = &tok;
				euid.tokId = tok.tokId;
			}
			break;
		}
	}

	bool madeNew = true;
	if( !idTok_p || !euid.isValid() ) {
		d_curEnt = 0;
		return;
	}
	d_curEnt = &( dtaIdx->entPool.addOneEntity(madeNew,euid));

	//// may need to enter unique id to the entity as findable token
	if( d_elistFlags.isEuidTok ) {
		EntTokenOrderInfo ord;
		ord.setBit_Euid();
		TokenEntityLinkInfo teli;
		teli.setBit_Euid();
		dtaIdx->addTokenToEntity( *idTok_p, *d_curEnt, ord, teli );
	}
}

void EntityLoader_XML::handle_entity_close( )
{
	d_curEnt = 0;
	d_curTokOrdInfo= EntTokenOrderInfo();
	
}

void EntityLoader_XML::handle_name_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz )
{
	if( parentTag == TAG_NAME ) 
		d_curTokOrdInfo.incrementName();
}

void EntityLoader_XML::handle_name_close( )
{
	d_curTokOrdInfo= EntTokenOrderInfo();
	d_curTok = 0;
}

void EntityLoader_XML::handle_entlist_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz )
{
	/// trying to set the default entity class information
	/// as well as anything else entities in the list have in common
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'c':
			if( !n[1] ) d_eclass.ec = atoi(v);
			break;
		case 's':
			if( !n[1] ) d_eclass.subclass = atoi(v);
			break;
		case 'i':
			if( !n[1] )  
				d_elistFlags.isEuidTok = true;
			break;
		}
	}
}
void EntityLoader_XML::handle_entlist_close( )
{
	resetParsingContext();
}

void EntityLoader_XML::handle_token_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz )
{
	uint32_t compTokId = INVALID_STORED_ID;
	const char* actualTok = 0;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'c':
			if( n[1] == 'i' )  // compounded token
				compTokId = atoi(v);
			break;
		case 's': 
			if( !n[1] ) { // strength
				d_curTELI.strength = TokenEntityLinkInfo::getValidInt8(atoi(v));
			} else if( n[1] == 't' && !n[2] ) { // st .. stem
				d_curTELI.setBit_Stem( );
			}
			break;
		case 't':
			if( !n[1] )  // t - token
				actualTok = v;
			break;
		case 'i':
			if( !n[1] ) {// inidicative
				d_curTELI.indicative = TokenEntityLinkInfo::getValidInt8(atoi(v));
			}
			break;
		case 'm':
			if( !n[1] ) // misspell
				d_curTELI.setBit_Misspell( );

			break;
		}
	}
	if( actualTok ) { // adding token right away
		bool newAdded = true;
		StoredToken& tok = dtaIdx->tokPool.addSingleTok( newAdded, actualTok );
		d_curTok = &tok;
	}
}
void EntityLoader_XML::handle_token_close( )
{
	if( !d_curTok || !d_curEnt ) 
		return;
	dtaIdx->addTokenToEntity( *d_curTok, *d_curEnt, d_curTokOrdInfo, d_curTELI );

	// cleanup
	d_curTok = 0;
	d_curTokOrdInfo = EntTokenOrderInfo();
	d_curTELI = TokenEntityLinkInfo();
}
// attr_sz number of attribute pairs
void EntityLoader_XML::startElement( const char* tag, const char_cp * attr, size_t attr_sz )
{
	Tag_t tid = getTag( tag );
	Tag_t parentTag = ( tagStack.empty() ? TAG_NULL:  tagStack.top() );

	tagStack.push( tid );
	switch( tid ) {
	case TAG_ENTITY: handle_entity_open(parentTag,attr,attr_sz); break;
	case TAG_TOKEN: handle_token_open(parentTag,attr,attr_sz); break;
	case TAG_NAME: handle_name_open(parentTag,attr,attr_sz); break;
	case TAG_ENTLIST: handle_entlist_open(parentTag,attr,attr_sz); break;
	default:
		break;
	}
}

void EntityLoader_XML::endElement( const char* tag )
{
	Tag_t tid = getTag( tag );

	tagStack.pop();
	switch( tid ) {
	case TAG_ENTITY: handle_entity_close(); break;
	case TAG_TOKEN: handle_token_close(); break;
	case TAG_NAME: handle_name_close(); break;
	case TAG_ENTLIST: handle_entlist_close(); break;
	default:
		break;
	}
}

void EntityLoader_XML::tokenizeAddText_TokOrName( const char* txt, int l )
{
	if( l<=0 )
		return;
	size_t len = (size_t)l;
	if( d_txtTmpVec.capacity() < len ) 
		d_txtTmpVec.reserve( len );
	
	d_txtTmpVec.resize( len+1 );
	memcpy( &d_txtTmpVec[0], txt, len );
	d_txtTmpVec.back() = 0;
	
	const char* str = &d_txtTmpVec[0];
	for( std::vector<char>::iterator i = d_txtTmpVec.begin(); i != d_txtTmpVec.end(); ++i )
	{
		if( isspace(*i) ) { 
			*i = 0;
			if( *str ) {
				dtaIdx->addTokenToEntity( str, *d_curEnt, d_curTokOrdInfo, d_curTELI );
				d_curTokOrdInfo.incrementIdx();
			}
			str=&(*i)+1;
		}
	}
	if( *str ) {
		dtaIdx->addTokenToEntity( str, *d_curEnt, d_curTokOrdInfo, d_curTELI );
		d_curTokOrdInfo.incrementIdx();
	}
}

void EntityLoader_XML::getElementText( const char* txt, int len )
{
	if( len<=0 ) 
		return;
	/// in case multiple tokens are encountered in the text 
	/// 
	if( !d_curEnt ) 
		return;

	if( tagStack.empty() ) 
		return ;
	Tag_t tag =tagStack.top() ;
	switch( tag ) {
	case TAG_TOKEN:
	case TAG_NAME:
		tokenizeAddText_TokOrName(txt,len);
		break;
	default: 
		break;
	}
}

int EntityLoader_XML::init()
{
	if( parser ) {
		XML_ParserFree(parser);
		parser = 0;
	}
	parser = XML_ParserCreate(NULL);
	if( !parser )
		return 666;
	XML_SetUserData(parser,this);

	// cast to XML_StartElementHandler
	// cast to XML_EndElementHandler
	// cast to XML_CharacterDataHandler
	XML_SetElementHandler(parser, 
		::startElement, ::endElement);
	XML_SetCharacterDataHandler(parser, charDataHandle);
	
	return 0;
}

void EntityLoader_XML::resetParsingContext()
{
	d_eclass.reset();
	d_curEnt=0;
	d_curTok=0;
	d_curTokOrdInfo = EntTokenOrderInfo();
	d_curTELI = TokenEntityLinkInfo();
}

EntityLoader_XML::EntityLoader_XML(DtaIndex* di):
	parser(0),
	dtaIdx(di),
	d_curEnt(0),
	d_curTok(0)
{
}

EntityLoader_XML::~EntityLoader_XML()
{
	if( parser )
		XML_ParserFree(parser);
}

int EntityLoader_XML::readFile( const char* fileName )
{
	init();
	if( !parser ) {
		std::cerr << "FATAL cant create XML parser\n";
		return ELXML_ERR_PARSER;
	}
	FILE* fp = stdin;
	if( fileName ) {
		fp = fopen( fileName, "r" );
		if( !fp ) {
			std::cerr << "Failed to open \"" << fileName << "\" for reading\n";
			return ELXML_ERR_FILE;
		}
		std::cerr << "reading entities from file " << fileName << "\n";
	}
  	
	char buf[ 128*1024 ];	
	bool done = false;
	do {
    	size_t len = fread(buf, 1, sizeof(buf), fp);
    	done = len < sizeof(buf);
    	if (!XML_Parse(parser, buf, len, done)) {
      		fprintf(stderr,
	      		"%s at line %d\n",
	      		XML_ErrorString(XML_GetErrorCode(parser)),
	      		(int)XML_GetCurrentLineNumber(parser));
      		return 1;
    	}
	} while (!done);
	return ELXML_ERR_OK;
}

} // namespace barzer
