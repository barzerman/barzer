#ifndef BARZER_EL_PARSER_H
#define BARZER_EL_PARSER_H

#include <barzer_el_trie.h>
#include <iostream>
#include <vector>
#include <cstdio>
#include <string>
#include <barzer_el_btnd.h>
#include <ay/ay_pool_with_id.h>

/// wrapper object for various formats in which BarzEL may be fed to the application
/// as well as the data structures required for the actual parsing
namespace barzer {

class StoredUniverse;
class GlobalPools;
//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS 


/// statement parse tree represents a single BarzEL  statement as parsed on load 
/// this tree is evaluated and resulting paths are added to a barzel trie by the parser
struct BELStatementParsed {
	size_t d_stmtNumber;
	std::string d_sourceName;
	uint32_t d_sourceNameStrId; 

	BELStatementParsed() : 
		d_stmtNumber(0),d_sourceNameStrId(0xffffffff)
	{}

	BELParseTreeNode pattern; // points at the node under statement
	BELParseTreeNode translation; // points at the node under statement 
	void clear()
		{ pattern.clear(); translation.clear(); }
	
	void setSrcInfo( const char* srcName, uint32_t strId ) 
	{ 
		d_stmtNumber = 0;
		d_sourceNameStrId = strId;
		d_sourceName.assign( srcName );
	}
	void stmtNumberIncrement() { ++d_stmtNumber; }

	uint32_t getSourceNameStrId()  const { return d_sourceNameStrId; }
	size_t getStmtNumber() const { return d_stmtNumber; }
	const std::string&  getSourceName() const { return d_sourceName; }
};

/// all specific parsers inherit from this base type and overload 
/// parse method. Specifically the XML file parser 
/// in the future when BarzEL has its own language we will add a new one 
/// there will also be a binary parser 

class BELReader;
class BELTrie;
class BELParser {
protected:
	BELReader* reader; // parser doesnt own this pointer. reader owns the parser
	std::ostream& d_outStream;

	enum { MAX_VARNAME_LENGTH };
	uint32_t internString_internal( const char* s ) ;
	uint32_t internString( const char* s ) ;
	/// gets variable name v1.v2.v3 ... interns individual parts and 
	/// adds result to the variable pool as a whole vector
	uint32_t internVariable( const char* );
	/// alias can be 0 - in that case no alias will be generated and an orphaned compounded 
	/// token will be added 
	uint32_t addCompoundedWordLiteral( const char* alias );
	

	const BELReader* getReader() const { return reader; }
	BELReader* getReader() { return reader; }

	const BELTrie& getTrie() const; 
	BELTrie& getTrie(); 
public:
	uint32_t stemAndInternTmpText( const char* s, int len );
	BELParser( BELReader* r, std::ostream& outStream ) : reader(r), d_outStream(outStream) {}
	virtual int parse( std::istream& ) = 0;
	virtual ~BELParser() {}
	GlobalPools& getGlobalPools();
	const  GlobalPools& getGlobalPools() const;
	
	const BELParseTreeNode* getMacroByName( const std::string&  ) const;
};

//// the reader gets barzel code from a file 
///  parses them using the parser for  given format and adds 
///  resulting barzel statements to the trie 
/// 
class BarzelTrieNode;
class BELReader {
protected: 
	BELTrie* trie ; 
	BELParser* parser;
	GlobalPools &gp;
	size_t numStatements; /// total number of successfully read statements 

	std::string inputFileName; 
	
	bool silentMode;
public:
	bool isSilentMode() const { return silentMode; }
	void setSilentMode() { silentMode = true; }

	const std::string& getInputFileName() const { return inputFileName; }
	void setTrie( const std::string& trieClass, const std::string& trieId ) ;
	void setTrie(BELTrie *t) { trie = t; }

	BELTrie& getTrie() { return *trie ; }
	const BELTrie& getTrie() const { return *trie ; }

	GlobalPools& getGlobalPools() { return gp; }
	const GlobalPools& getGlobalPools() const { return gp; }


	/// barzEL input formats
	typedef enum {
		INPUT_FMT_AUTO, // will try to figure out from file extension . this only works when fileName !=0
		
		INPUT_FMT_XML,  // default
		INPUT_FMT_BIN,
		INPUT_FMT_BARZEL,

		INPUT_FMT_XML_EMITTER,

		INPUT_FMT_MAX
	} InputFormat;
	InputFormat inputFmt;

	BELReader( GlobalPools &g );

	BELReader( BELTrie* t, GlobalPools &g  )  :
		trie(t) , parser(0), gp(g), numStatements(0) , inputFmt(INPUT_FMT_XML)
	{}
	
	virtual ~BELReader() {
		delete parser;
	}

	/// this method is called by the parser for every statement tree 
	virtual void addStatement( const BELStatementParsed& );
	virtual void addMacro( const std::string& macro, const BELStatementParsed& );

	BELParser*  initParser(InputFormat fmt);
	int  loadFromStream( std::istream& fp );
	//   combines initParser and LoadFromStream 
	//   creates the parser and calls loadFromStream
	int  loadFromFile( const char* fileName, InputFormat fmt = INPUT_FMT_XML );

	std::ostream& printNode( std::ostream& fp, const BarzelTrieNode& node ) const;
};

inline BELTrie& BELParser::getTrie() { return reader->getTrie(); }
inline const BELTrie& BELParser::getTrie() const { return reader->getTrie(); }

inline GlobalPools& BELParser::getGlobalPools() { return reader->getGlobalPools(); }

inline const GlobalPools& BELParser::getGlobalPools() const { return reader->getGlobalPools(); }

template<class T>class BELExpandReader : public BELReader {
	T &functor;
public:
	BELExpandReader(T &f, BELTrie* t, GlobalPools &gp )
		: BELReader(t,gp), functor(f) {}
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
