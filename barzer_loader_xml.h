#ifndef BARZER_LOADER_XML_H
#define BARZER_LOADER_XML_H

#include <stack>
#include <ay/ay_util_char.h>

struct XML_ParserStruct;
using ay::char_cp;
namespace barzer {
struct DtaIndex;

//// all defaults must be always 0 
struct EntityLoader_EntListFlags  {
	bool isEuidTok; // when true unique ids will be added as searcheable toks to the entity
	int32_t relevance; // default relevance for the entities 
	inline void reset()  
	{
		isEuidTok= false;
		relevance=0;
	}
	EntityLoader_EntListFlags() 
	{reset();}
};

struct EntityLoader_XML {
	XML_ParserStruct* parser; // initialized in constructor, deleted in destru
	DtaIndex* dtaIdx;
	/// tags and attributes are case sensitive - everything is in lower case
	typedef enum {
		TAG_NULL,

		// single entity <e>
		// mandatory attributes
		// c(lass),s(ubclass)
		// i(d) - euid - entity id unique within class and subclass
		// free text will be tokenized on space only .. 
		// r(elevance) - tie breaker for the same tokens (relevance)
		TAG_ENTITY, 

		// collection of entities - top level tag <entlist>
		// valid attributes 
		// c(lass),s(ubclass) - class, subclass 
		// i(ds as tokens)  - when set unique ids will 
		// be added to entities as tokens
		// r(elevance) - default relevance 
		TAG_ENTLIST,

		// single token within entity (or name) <t>
		// free text will be tokenized on space only .. 
		// attributes - 
		// "c(ompound)i(d)" ~ id of compounded token
		// "st(em)" - this is stem only token fo given entity
		// "s(trength)" - token-entity strength
		// "t(oken)" - optional - actual token. may be also in the tag text
		// "i(ndicative)" - token-ent indicativeness
		// "m(isspelling)"
		TAG_TOKEN,   

		// UNIMPLEMENTED - names will be implemented later
		// name is a collection of tokens. it's contained in entity <n>
		TAG_NAME,    // name is a collection of tokens. it's contained in entity <n>

		TAG_MAX
	} Tag_t;

	std::stack< Tag_t > tagStack;

	enum { INVALID_POSITION =-1};
	/// parsing context 
	EntityLoader_EntListFlags      d_elistFlags;

	StoredEntityClass d_eclass; // default eclass (may be set at the entlist level)
	StoredEntity* d_curEnt;  /// current entity 
	StoredToken* d_curTok;  /// current token
	TokenEntityLinkInfo d_curTELI; // lasts thru the token
	EntTokenOrderInfo d_curTokOrdInfo; /// lasts thru the name

	std::vector<char> d_txtTmpVec; // used for tokenization. state unimportant

	void resetParsingContext(); // called from entlist close
	//// end of parsing context stuff
	
	void handle_entity_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz );
	void handle_entity_close( );
	void handle_name_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz );
	void handle_name_close( );
	void handle_entlist_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz );
	void handle_entlist_close( );
	void handle_token_open( Tag_t parentTag, const char_cp * attr, size_t attr_sz );
	void handle_token_close( );
	
	void tokenizeAddText_TokOrName( const char* txt, int len );
	Tag_t getTag( const char* s ) const;
	// attr_sz number of attribute pairs
	void startElement( const char* tag, const char_cp * attr, size_t attr_sz );
	void endElement( const char* tag );
	void getElementText( const char* txt, int len );

	enum {
		ELXML_ERR_OK,		// no error
		ELXML_ERR_PARSER,   // failed to create parser
		ELXML_ERR_FILE   // failed to open file
	};
	EntityLoader_XML(DtaIndex*);
	// initializes the parser. returns 0 if success
	int init();

	// returns ELXML_ERR_XXXX
	int readFile( const char* fileName );

	~EntityLoader_XML();
};

} // namespace barzer

#endif // BARZER_LOADER_XML_H
