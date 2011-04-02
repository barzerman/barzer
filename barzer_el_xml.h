#ifndef BARZER_EL_XML_H
#define BARZER_EL_XML_H

#include <barzer_el_parser.h>

#include <ay/ay_util_char.h>
using ay::char_cp;
struct XML_ParserStruct;

namespace barzer {
//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 

class BELParserXML : public BELParser {
public:
	/// there's practically a 1-1 correspondence between the BELParseTreeNode::nodeType enums and 
	/// the XML tags. the TreeNode enums are not sequential so we will address this issue 
	/// but assigning ranges 
	enum {
		/// control tags 
		TAG_STATEMENT, // doesnt have a pair in the node type realm - the whole statement
		
		TAG_PATTERN, // no pair in the node type - represents the whole left side 

		TAG_RANGE_ELEMENT, // no pair in the node type - represents the whole left side 
					 // BEL_PATTERN_XXX = TAG_XXX - TAG_PATTERN

		TAG_T,		// token
		TAG_TG,		// tg
		TAG_P, 		// punctuation
		TAG_SPC, 	// SPACE
		TAG_N, 		// number
		TAG_RX, 	// token regexp
		TAG_TDRV, 	// token derivaive
		TAG_WC, 	// word class
		TAG_W, 		// token wildcard
		TAG_DATE, 	// date
		TAG_TIME, 	// time

		TAG_RANGE_STRUCT, // not a real tag used for translation
						// BEL_STRUCT_XXX = TAG_XXX-TAG_RANGE_STRUCT
		
		TAG_LIST, // sequence of elements
		TAG_ANY,  // any element 
		TAG_OPT,  // optional subtree
		TAG_PERM, // permutation of children
		TAG_TAIL, // if children are A,B,C this translates into [A], [A,B] and [A,B,C]

		TAG_TRANSLATION, // no pair in the node type - represents the whole left side 
		TAG_REWRITE_STRUCT, // not a real tag used for translation
						// BEL_REWRITE_XXX = TAG_XXX-TAG_REWRITE_STRUCT

		TAG_RL, // literal
		TAG_RN, // number
		TAG_VAR, // variable
		TAG_FUNC, // function
		
		TAG_MAX

	};

	XML_ParserStruct* parser;
	BELParserXML( BELReader* r ) : 
		BELParser(r),parser(0) {}

	// actual expat entry points 
	void startElement( const char* tag, const char_cp * attr, size_t attr_sz );
	void endElement( const char* tag );
	void getElementText( const char* txt, int len );

	int parse( std::istream& );
};

}

#endif // BARZER_EL_XML_H
