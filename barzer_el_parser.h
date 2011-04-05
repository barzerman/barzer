#ifndef BARZER_EL_PARSER_H
#define BARZER_EL_PARSER_H

#include <barzer_el_trie.h>
#include <iostream>
#include <vector>
#include <cstdio>
#include <string>
#include <barzer_el_btnd.h>

/// wrapper object for various formats in which BarzEL may be fed to the application
/// as well as the data structures required for the actual parsing
namespace barzer {

//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 

//// tree of these nodes is built during parsing
//// a the next step the tree gets interpreted into BrzelTrie path and added to a trie 
struct BELParseTreeNode {
	/// see barzer_el_btnd.h for the structure of the nested variant 
	BTNDVariant btndVar;  // node data

	typedef std::vector<BELParseTreeNode> ChildrenVec;

	ChildrenVec child;
	
	BELParseTreeNode( ) 
	{}

	template <typename T>
	BELParseTreeNode& addChild( const T& t) {
		child.resize( child.size() +1 );
		child.back().btndVar = t;
		return child.back();
	}
	void clear( ) 
	{
		child.clear();
		btndVar = BTND_None();
	}

	template <typename T>
	T& setNodeData( const T& d ) 
		{ return( btndVar = d, boost::get<T>(btndVar) ); }
	
	template <typename T>
	void getNodeDataPtr( T*& ptr ) 
		{ ptr = boost::get<T>( &btndVar ); }
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
	BELParseTreeNode translation; // points at the node under statement 
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
	ay::UniqueCharPool* strPool;
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

	BELReader( BELTrie* t, ay::UniqueCharPool* sPool )  :
		trie(t) , parser(0), strPool(sPool), numStatements(0) , inputFmt(INPUT_FMT_XML)
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
