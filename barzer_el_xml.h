#ifndef BARZER_EL_XML_H
#define BARZER_EL_XML_H

#include <barzer_el_parser.h>

#include <ay/ay_bitflags.h>
#include <ay/ay_util_char.h>
#include <ay/ay_logger.h>
#include <stack>
#include <barzer_storage_types.h>
using ay::char_cp;

extern "C" {
struct XML_ParserStruct; // this is expat xml parser type. expat is in C
}

namespace barzer {
//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 

class BELParserXML : public BELParser {
	/// given a tag returns one of the TAG_XX enums (see below)
	int getTag( const char* s ) const;
	const char* getTagName( const char* s );
	bool isValidTag( int tag, int parent ) const;
	void processAttrForStructTag( BTND_StructData& node, const char_cp * attr, size_t attr_sz );
public:
	/// there's practically a 1-1 correspondence between the BELParseTreeNode::nodeType enums and 
	/// the XML tags. the TreeNode enums are not sequential so we will address this issue 
	/// but assigning ranges 
	enum {
		TAG_UNDEFINED,
		/// control tags 
		TAG_STATEMENT, // <stmt> doesnt have a pair in the node type realm - the whole statement
		
		TAG_PATTERN, // <pat> no pair in the node type - represents the whole left side 

		TAG_RANGE_ELEMENT, // no pair in the node type - represents the whole left side 
					 // BEL_PATTERN_XXX = TAG_XXX - TAG_PATTERN

		TAG_T,		// token
		TAG_TG,		// tg
		TAG_P, 		// punctuation
		TAG_SPC, 	// SPACE
		/// <n> . valid attributes
		/// l - low value, h - high, r - real .. no attributes means "any integer"
		TAG_N, 		// number
		TAG_RX, 	// token regexp
		TAG_TDRV, 	// token derivaive
		TAG_WCLS, 	// word class
		TAG_W, 		// token wildcard
		TAG_DATE, 	// date
		TAG_DATETIME, 	// date
		TAG_ENTITY, 	// entity or erc matched on entity 
		TAG_RANGE, 	// entity or erc matched on entity 
		TAG_ERCEXPR, 	// expression made of ERCs
		TAG_ERC, 	// single ERC
		TAG_EXPAND, // expands pre-defined macro
		TAG_TIME, 	// time

		TAG_RANGE_STRUCT, // not a real tag used for translation
						// BEL_STRUCT_XXX = TAG_XXX-TAG_RANGE_STRUCT
		
		TAG_LIST, // sequence of elements
		TAG_ANY,  // any element 
		TAG_OPT,  // optional subtree
		TAG_PERM, // permutation of children
		TAG_TAIL, // if children are A,B,C this translates into A,AB,ABC
		TAG_SUBSET, // if children are A,B,C this translates into A,B,C,AB,AC,BC,ABC

		TAG_TRANSLATION, // <trans> no pair in the node type - represents the whole left side 
		TAG_REWRITE_STRUCT, // not a real tag used for translation
						// BEL_REWRITE_XXX = TAG_XXX-TAG_REWRITE_STRUCT

		// <ltlr> valid attribute 
		// 
		TAG_LITERAL, // <ltrl>
		TAG_RNUMBER, // <rn>
		TAG_MKENT, // <mkent c="class" sc="subclass" id="UNIQID"> 
		TAG_NV, // <nv n="somename" v="somevalue">

		TAG_VAR, // <var>
		TAG_FUNC, // function
		TAG_SELECT,
		TAG_CASE,
		TAG_AND,
		TAG_OR,
		TAG_NOT,
		TAG_TEST,
		TAG_COND,

		TAG_STMSET, // document element
		
		TAG_MAX

	};

	XML_ParserStruct* parser;
	int statementCount;  // siply counts statetent tags for err diag
	std::stack< int > tagStack;

	struct CurStatementData {
		BELStatementParsed stmt;
		// std::string macroName;
        uint32_t macroNameId;
		uint32_t procNameId;

        StoredEntityUniqId d_curEntity;        

		enum {
			BIT_HAS_PATTERN,
			BIT_HAS_TRANSLATION,
			BIT_HAS_STATEMENT,
			BIT_IS_MACRO,  // is macro (no translation - pattern only)
			BIT_IS_PROC,   // is "stored procedure" - all translation, no pattern
			BIT_IS_INVALID,

			BIT_MAX
		};
		ay::bitflags<BIT_MAX> bits;
		enum {
			STATE_BLANK,
			STATE_PATTERN,
			STATE_TRANSLATION
		} state;
			
		CurStatementData() : macroNameId(0xffffffff),procNameId(0xffffffff), state(STATE_BLANK) {}
		void clear(); 

		std::stack< BELParseTreeNode* > nodeStack;
        
        const StoredEntityUniqId& getCurEntity() const { return d_curEntity; }
        void clearCurEntity() { d_curEntity = StoredEntityUniqId(); }
        void setCurEntity( const StoredEntityUniqId& euid ) { d_curEntity = euid; }
		BELParseTreeNode* getCurTreeNode()
		{
			if( nodeStack.empty() ) {
				if( state == STATE_TRANSLATION ) 
					return &(stmt.translation);
				else if( state == STATE_PATTERN ) 
					return &(stmt.pattern);
				else 
					return 0;
			} else 
				return nodeStack.top();
		}
		BELParseTreeNode* getCurrentNode() {
			return getCurTreeNode();
		}
		void setStatement() { 
			bits.set(BIT_HAS_STATEMENT);
		}

		void setTranslation() { 
			state = STATE_TRANSLATION;
			bits.set(BIT_HAS_TRANSLATION);
			stmt.translation.setNodeData( BTND_RewriteData() );
		}

		void setInvalid() { bits.set(BIT_IS_INVALID); }
		bool isValid() { return !bits[ BIT_IS_INVALID ] ; }
		void setPattern() { 
			state = STATE_PATTERN;
			bits.set(BIT_HAS_PATTERN); 
			stmt.pattern.setNodeData( BTND_StructData() );
		}
		// this is not the same a clear - this is the state of the XML parsing 
		// when endElement() for pattern/translate should call this 
		void setBlank() { 
			state = STATE_BLANK; 
            clearCurEntity();
		}
        bool isBlank() const { return(state==STATE_BLANK) ; }
		
		template <typename T>
		BELParseTreeNode* pushNode( const T& t )
		{
			//AYTRACE("pushNode");
			BELParseTreeNode* curNode = getCurTreeNode();
			if( !curNode )
				return 0;
			
			curNode = &(curNode->addChild(t));
			nodeStack.push( curNode );
			return curNode;
		}
		void popNode()
		{ 
			if( !nodeStack.empty() ) 
				nodeStack.pop(); 
		}

		void setProc(uint32_t strId ) {
			procNameId = strId;
			bits.set( BIT_IS_PROC );
		}
		void setMacro( uint32_t i ) 
        {
            macroNameId = i;
            bits.set( BIT_IS_MACRO );
        }
        /*
		void setMacro(const char* s ) {
			macroName.assign(s);
			bits.set( BIT_IS_MACRO );
		}
        */

		bool hasStatement() const { return bits[BIT_HAS_STATEMENT];};
		bool hasPattern() const { return bits[BIT_HAS_PATTERN];};
		bool hasTranslation() const { return bits[BIT_HAS_TRANSLATION];};
		bool isMacro() const { return bits[BIT_IS_MACRO];};
		bool isProc() const { return bits[BIT_IS_PROC];};
        
        bool isState_Blank() const { return (state == STATE_BLANK); }
        bool isState_Translation() const { return (state == STATE_TRANSLATION); }
        bool isState_Pattern() const { return (state == STATE_PATTERN); }
	} statement;

	mutable std::string d_tmpText; // used by getElementText as a temp buffer

	~BELParserXML() ;
	BELParserXML( BELReader* r);

	uint32_t internTmpText( const char* s, int len, bool stem, bool isNumeric ) 
		{ 
            uint32_t strId ;
            if( stem ) {
			    strId = stemAndInternTmpText( d_tmpText.assign(s,len).c_str(), len) ;
            } else {
                StoredToken& sTok = internString(d_tmpText.assign(s,len).c_str(),false);
                if( isNumeric && !sTok.classInfo.isNumber() ) 
                    sTok.classInfo.setNumber();
                
			    strId = sTok.getStringId();
            }
			return strId;
		}
	const char* setTmpText( const char* s, int len ) 
		{ return d_tmpText.assign( s, len ).c_str(); }

	// actual expat entry points 
	// tid is tag id. this function is called by (start/end)Element 
	void elementHandleRouter( int tid, const char_cp * attr, size_t attr_sz, bool close );

	// call elementHandleRouter
	void startElement( const char* tag, const char_cp * attr, size_t attr_sz );
	void endElement( const char* tag );

	void getElementText( const char* txt, int len );
	
	/// tag handles 
	void taghandle_STATEMENT( const char_cp * attr, size_t attr_sz, bool close=false );
	void taghandle_UNDEFINED( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_PATTERN( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_TRANSLATION( const char_cp * attr, size_t attr_sz , bool close=false);

	void taghandle_T( const char_cp * attr, size_t attr_sz , bool close=false);

	void taghandle_TG( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_P( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_SPC( const char_cp * attr, size_t attr_sz , bool close=false);

	void taghandle_N( const char_cp * attr, size_t attr_sz , bool close=false);

	void taghandle_RX( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_TDRV( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_WCLS( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_W( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_DATE( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_DATETIME( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_RANGE( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_ENTITY( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_ERC( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_ERCEXPR( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_EXPAND( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_TIME( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_LIST( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_ANY( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_OPT( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_PERM( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_TAIL( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_SUBSET( const char_cp * attr, size_t attr_sz , bool close=false);

	// <rn>
	void taghandle_RNUMBER( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_MKENT( const char_cp * attr, size_t attr_sz , bool close=false);
    /// name value pair (can be inside entity or on its own)
    /// attributes n,v for nme and value 
	void taghandle_NV( const char_cp * attr, size_t attr_sz , bool close=false);


	void taghandle_LITERAL( const char_cp * attr, size_t attr_sz , bool close=false);

	void taghandle_FUNC( const char_cp * attr, size_t attr_sz , bool close=false);
	void taghandle_VAR( const char_cp * attr, size_t attr_sz , bool close=false);
	
    void taghandle_SELECT( const char_cp * attr, size_t attr_sz , bool close=false);
    void taghandle_CASE( const char_cp * attr, size_t attr_sz , bool close=false);
    void processLogic( int , bool);

	// document element
	void taghandle_STMSET( const char_cp * attr, size_t attr_sz , bool close=false);

	int parse( std::istream& );
};

}
#endif // BARZER_EL_XML_H
