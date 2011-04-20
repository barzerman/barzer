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

class BELReader;

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
class BarzelTrieNode;
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
	
	~BELReader() {
		delete parser;
	}

	/// this method is called by the parser for every statement tree 
	virtual void addStatement( const BELStatementParsed& );

	BELParser*  initParser(InputFormat fmt);
	int  loadFromStream( std::istream& fp );
	//   combines initParser and LoadFromStream 
	//   creates the parser and calls loadFromStream
	int  loadFromFile( const char* fileName, InputFormat fmt = INPUT_FMT_XML );

	std::ostream& printNode( std::ostream& fp, const BarzelTrieNode& node ) const;
};

template<class T>class BELExpandReader : public BELReader {
	T &functor;
public:
	BELExpandReader(T &f, BELTrie* t, ay::UniqueCharPool* sPool)
		: BELReader(t, sPool), functor(f) {}
	void addStatement( const BELStatementParsed& sp)
	{
		BELParseTreeNode_PatternEmitter emitter( sp.pattern );
		do {
			const BTND_PatternDataVec& seq = emitter.getCurSequence();
			functor(seq);
		} while( emitter.produceSequence() );
		++numStatements;
	}
};

}

#endif // BARZER_EL_PARSER_H
