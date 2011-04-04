#ifndef BARZER_EL_PARSER_H
#define BARZER_EL_PARSER_H

#include <barzer_el_trie.h>
#include <iostream>
#include <vector>
#include <cstdio>
#include <string>

/// wrapper object for various formats in which BarzEL may be fed to the application
/// as well as the data structures required for the actual parsing
namespace barzer {

//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 

//// tree of these nodes is built during parsing
//// a the next step the tree gets interpreted into BrzelTrie path and added to a trie 
struct BELParseTreeNode {
	typedef enum {
		NODE_UNDEFINED,

		NODE_PATTERN_ELEMENT,   // pattern only
		NODE_PATTERN_STRUCTURE, // pattern only
		NODE_REWRITE
	} NodeClass;
	NodeClass nodeClass;
	
	enum {
		BEL_TYPE_UNDEFINED =0
	};
//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 
	enum {
		BEL_PATTERN_UNDEFINED=BEL_TYPE_UNDEFINED,

		BEL_PATTERN_T,		// token
		BEL_PATTERN_TG,		// tg
		BEL_PATTERN_P, 		// punctuation
		BEL_PATTERN_SPC, 	// SPACE
		BEL_PATTERN_N, 		// number
		BEL_PATTERN_RX, 	// token regexp
		BEL_PATTERN_TDRV, 	// token derivaive
		BEL_PATTERN_WC, 	// word class
		BEL_PATTERN_W, 		// token wildcard
		BEL_PATTERN_DATE, 	// date
		BEL_PATTERN_TIME, 	// time

		/// add new types above this line only
		BEL_PATTERN_MAX
	} ;

//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 
	/// pattern structure types 
	enum {
		BEL_STRUCT_UNDEFINED=BEL_TYPE_UNDEFINED, 

		BEL_STRUCT_LIST, // sequence of elements
		BEL_STRUCT_ANY,  // any element 
		BEL_STRUCT_OPT,  // optional subtree
		BEL_STRUCT_PERM, // permutation of children
		BEL_STRUCT_TAIL, // if children are A,B,C this translates into [A], [A,B] and [A,B,C]
		/// add new types above this line only
		BEL_STRUCT_MAX
	} ;

//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 
	// translation types
	enum {
		BEL_REWRITE_UNDEFINED=BEL_TYPE_UNDEFINED,

		BEL_REWRITE_LITERAL,
		BEL_REWRITE_NUMBER,
		BEL_REWRITE_VARIABLE,
		BEL_REWRITE_FUNCTION,
		
		BEL_REWRITE_MAX
	} ;
	
	int nodeType; /// depending on the class one of the above enums
	
	typedef std::vector<BELParseTreeNode> ChildrenVec;

	ChildrenVec child;
	BELParseTreeNodeData nodeData; 
	
	BELParseTreeNode( ) : 
		nodeClass( NODE_UNDEFINED), 
		nodeType( BEL_TYPE_UNDEFINED ) 
	{}
	// takes node class and node type as parameters
	BELParseTreeNode( NodeClass nc, int nt ) : 
		nodeClass(nc), nodeType(nt)
	{}
	void setClassAndType( NodeClass nc, int nt ) { nodeClass =nc; nodeType = nt; }

	BELParseTreeNode& addChild( NodeClass nc, int nt ) {
		child.resize( child.size() +1 );
		child.back().setClassAndType(nc,nt);
		return child.back();
	}
	void clear( ) 
	{
		child.clear();
		setClassAndType(NODE_UNDEFINED,BEL_TYPE_UNDEFINED);
	}
};

/// statement parse tree represents a single BarzEL  statement as parsed on load 
/// this tree is evaluated and resulting paths are added to a barzel trie by the parser
struct BELStatementParsed {
	struct Data {
		uint32_t id; // 
		int strength; // 

		Data() : id(0xffffffff), strength(0) {}
		void clear()
		{ id=0xffffffff;strength = 0; }
	};
	BELParseTreeNode pattern; // points at the node under statement
	BELParseTreeNode translation; // poitns at the node under statement 
	void clear()
		{ pattern.clear(); translation.clear(); }
};

/// all specific parsers inherit from this base type and overload 
/// parse method. Specifically the XML file parser 
/// in the future when BarzEL has its own language we will add a new one 
/// there will also be a binary parser 

struct BELReader;

class BELParser {
protected:
	BELReader* reader; // parser doesnt own this pointer. reader owns the parser
public:
	BELParser( BELReader* r ) : reader(r) {}
	virtual int parse( std::istream& ) = 0;
	virtual ~BELParser() {}
};

struct BELTrie;
//// the reader gets barzel code from a file 
///  parses them using the parser for  given format and adds 
///  resulting barzel statements to the trie 
/// 
class BELReader {
protected: 
	BELTrie* trie ; 
	BELParser* parser;
	size_t numStatements; /// total number of successfully read statements 

	std::string inputFileName; 

public:

	/// barzEL input formats
	typedef enum {
		INPUT_FMT_AUTO, // will try to figure out from file extension . this only works when fileName !=0
		
		INPUT_FMT_XML,  // default
		INPUT_FMT_BIN,
		INPUT_FMT_BARZEL,

		INPUT_FMT_MAX
	} InputFormat;
	InputFormat inputFmt;

	BELReader( BELTrie* t )  :
		trie(t) , parser(0), numStatements(0) , inputFmt(INPUT_FMT_XML)
	{}
	
	/// this method is called by the parser for every statement tree 
	void addStatement( const BELStatementParsed& );

	BELParser*  initParser(InputFormat fmt);
	int  loadFromStream( std::istream& fp );
	//   combines initParser and LoadFromStream 
	//   creates the parser and calls loadFromStream
	int  loadFromFile( const char* fileName, InputFormat fmt = INPUT_FMT_XML );
}; 

}

#endif // BARZER_EL_PARSER_H
