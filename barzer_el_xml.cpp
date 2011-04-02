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
int BELParserXML::BELParserXML::parse( std::istream& )
{
#warning BELParserXML::parse unimplemented
return 666;
}

}
