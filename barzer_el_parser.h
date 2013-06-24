
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <iostream>
#include <vector>
#include <cstdio>
#include <set>
#include <string>
//#include <barzer_emitter.h>
#include <barzer_el_btnd.h>
#include <ay/ay_pool_with_id.h>
#include <ay/ay_xml_util.h>
#include <ay/ay_parse.h>

/// wrapper object for various formats in which BarzEL may be fed to the application
/// as well as the data structures required for the actual parsing
namespace barzer {

struct StoredToken;
class StoredUniverse;
class GlobalPools;
class BELReader;
struct BELParseTreeNode_PatternEmitter;
//// !!!!!!!!!!! NEVER REORDER ANY OF THE ENUMS

/// statement parse tree represents a single BarzEL  statement as parsed on load
/// this tree is evaluated and resulting paths are added to a barzel trie by the parser
struct BELStatementParsed {
    enum { 
        EXEC_MODE_REWRITE,  // standard 
        EXEC_MODE_IMPERATIVE_PRE,  // imperative befiore rewriting
        EXEC_MODE_IMPERATIVE_POST  // imperative after rewriting
    };
    int    d_execMode; // EXEC_MODE_XXXX

	uint32_t d_stmtNumber;
	std::string d_sourceName;
	uint32_t d_sourceNameStrId;
    enum {
        CLASH_OVERRIDE_NONE,   // n (default)
        CLASH_OVERRIDE_APPEND, // a - tries to ambiguate by appending translation result
        CLASH_OVERRIDE_TAKELAST // y last wins
    };
    int     d_ruleClashOverride; // when true will override the ruleclash and se the latest translation (stmt attribute "o")

    BELReader* d_reader;
    int d_tranUnmatchable;
    int d_confidenceBoost;

	BELStatementParsed();
	BELParseTreeNode pattern; // points at the node under statement
	BELParseTreeNode translation; // points at the node under statement

    void setExecImperative( bool pre )
        { d_execMode = ( pre ? EXEC_MODE_IMPERATIVE_PRE : EXEC_MODE_IMPERATIVE_POST ); }
    void setExecRewrite()
        { d_execMode = EXEC_MODE_REWRITE;}

    int getExecMode() const { return d_execMode; }
    bool isImperativeExec() const { return d_execMode!= EXEC_MODE_REWRITE; }
	void clear();
    BELReader* getReader() const { return const_cast<BELReader*>(d_reader); }
    std::ostream& getErrStream() const;

    void setReader( BELReader* rdr ) { d_reader = rdr; }
	void setSrcInfo( const char* srcName, uint32_t strId )
	{
		d_stmtNumber = 0;
		d_sourceNameStrId = strId;
		d_sourceName.assign( srcName );
	}

    void setRuleClashOverride( const char* str ) ;
    int ruleClashOverride( ) const { return (d_ruleClashOverride); }

	void stmtNumberIncrement() { ++d_stmtNumber; }

	size_t setStmtNumber( size_t num = 0);

	uint32_t getSourceNameStrId()  const { return d_sourceNameStrId; }
	size_t getStmtNumber() const { return d_stmtNumber; }
	const std::string&  getSourceName() const { return d_sourceName; }

    int isTranUnmatchable() const { return d_tranUnmatchable;  }
    void setTranUnmatchable() { d_tranUnmatchable = 1; }
    int getTranConfidenceBoost() const { return d_confidenceBoost; }
    void setTranConfidenceBoost(int x) { d_confidenceBoost = x; }
    void clearUnmatchable() { d_tranUnmatchable= 0; }
};

/// all specific parsers inherit from this base type and overload
/// parse method. Specifically the XML file parser
/// in the future when BarzEL has its own language we will add a new one
/// there will also be a binary parser

class BELReader;
class BELTrie;
class BarzelEvalNode;
class BELParser {
protected:
	BELReader* reader; // parser doesnt own this pointer. reader owns the parser

	enum { MAX_VARNAME_LENGTH };
	uint32_t internString_internal( const char* s ) ;

	/// unstemSrc is not null if s is stem. In this case, unstemSrc points to the original
	/// unstemmed version of the s.
	StoredToken& internString( int lang, const char* s, bool noSpell, const char* unstemSrc ) ;

	/// gets variable name v1.v2.v3 ... interns individual parts and
	/// adds result to the variable pool as a whole vector
	uint32_t internVariable( const char* );
	/// alias can be 0 - in that case no alias will be generated and an orphaned compounded
	/// token will be added
	uint32_t addCompoundedWordLiteral( const char* alias );

public:
	const BELReader* getReader() const { return reader; }
	BELReader* getReader() { return reader; }

	const BELTrie& getTrie() const;
	BELTrie& getTrie();

	uint32_t stemAndInternTmpText( const char* s, int len );
	BELParser( BELReader* r ) ;
	virtual int parse( std::istream& ) = 0;
	virtual ~BELParser() {}
	GlobalPools& getGlobalPools();
	const  GlobalPools& getGlobalPools() const;

	const BELParseTreeNode* getMacroByName( const BELTrie& trie, const char* ) const;
	const BELParseTreeNode* getMacroByName( const char* ) const;
    const BELParseTreeNode* getMacroByNameId( uint32_t strId ) const;

	/// strId is internal token id of the proc name
	const BarzelEvalNode* getProcByName( uint32_t strId ) const;
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
	size_t numSkippedStatements; /// total of statements skipped
	size_t numMacros; /// total number of successfully loaded macros
	size_t numProcs; /// total number of successfully loaded procs
    size_t numEmits; // total currently accumulated number of emits for all rules processed
    int    d_defaultExecMode; // BELStatementParsed::EXEC_MODE_XXX


	std::string inputFileName;

	bool silentMode;

	/// priority of the words in the currently parsed trie fo spelling
	/// default trie - 0, user specific tries - 10
	uint8_t d_trieSpellPriority;

	/// the higher the priority the earlier this word would be in the spelling correction
	/// order
	/// if ruleset filename has substring '_fluff' in it priority will be deduced as the ruleset

	/// a number .. 0 for fluff 5 for everything else
	/// so this priority is 0,5,10,15
	uint8_t d_rulesetSpellPriority;

	/// by default is set to d_trieSpellPriority+ d_rulesetSpellPriority
	/// can be overridden (currently this is not done)
	uint8_t d_spellPriority;
    /// default - false . when true this means it's being called from a server command 
    /// then things such as produceWordVariants are always invoked etc
    bool    d_liveCommand; 

    std::set< std::string > d_tagFilter;
public:
    const std::set< std::string >& getTagFilter()  const { return d_tagFilter; }

    void    clearTagFilter() { d_tagFilter.clear(); }
    void    addTagToFilter(const std::string& s) { d_tagFilter.insert(s); }
    bool    hasTagFilters() const { return d_tagFilter.size(); }
    bool    tagsPassFilter( const char* tagStr ) const
    {
        std::set<std::string> tmpSet;
        ay::separated_string_to_set x(tmpSet);
        x( tagStr );

        return (tmpSet.size() ? ay::set_intersection_nonempty( tmpSet.begin(), tmpSet.end(), d_tagFilter.begin(), d_tagFilter.end() ) : false );
    }
    void    setTagFilter(const char* s) 
    { 
        d_tagFilter.clear();
        ay::separated_string_to_set x( d_tagFilter );
        x( s );
    }

    void    setDefaultExecMode( int m ) { d_defaultExecMode = m; }
    int     getDefaultExecMode() const { return d_defaultExecMode; }

    void setLiveCommandMode() { d_liveCommand= true; }
    bool isLiveCommandMode() const { return d_liveCommand; }

	/// barzEL input formats
	typedef enum {
		INPUT_FMT_AUTO, // will try to figure out from file extension . this only works when fileName !=0

		INPUT_FMT_XML,  // default
		INPUT_FMT_BIN,
		INPUT_FMT_BARZEL,

		INPUT_FMT_MAX
	} InputFormat;
	InputFormat inputFmt;
private:
	StoredUniverse *d_currentUniverse;
	/// false by deault, when true takes trie name and class id from
	bool d_trieIdSet;
	uint32_t d_curTrieId, d_curTrieClass;

    /// errors will be written to this stream
    std::ostream* d_errStream;

    size_t  d_maxEmitCountPerStatement, d_maxEmitCountPerTrie, d_maxStatementsPerTrie; /// max
    bool    d_respectLimits; /// when false get_maxEmitXXX returns max unsigned int
    uint8_t d_noCanonicalNames;
	/// both computeXXXSpellPriority functions update d_spellPriority
	/// deduces trie spell priority from trie class name ( "" - priority 0, otherwise - 10 )
	void computeImplicitTrieSpellPriority( uint32_t tc, uint32_t tid );
	/// deduces ruleset spelling priority from ruleset file name ( if has no "_fluff" substring then
	/// priority is 5 otherwise 0)
	void computeRulesetSpellPriority( const char* fileName );
    // blocker constructor
	BELReader( const BELReader& r) : gp(r.gp),d_respectLimits(true) {
        std::cerr << "PANIC: BELReader copy constructor invoked barzer_el_parser.h \n" ;
    }
public:
    bool isOk_EmitCountPerStatement( size_t x ) const 
        { return( d_respectLimits ? x< d_maxEmitCountPerStatement: true ); }
    bool  isOk_EmitCountPerTrie( size_t x ) const 
        { return ( d_respectLimits ? (x<d_maxEmitCountPerTrie) : true); }
    bool  isOk_StatementsPerTrie( size_t x ) const { return (d_respectLimits ? (x<d_maxStatementsPerTrie): true); }
    
    void setRespectLimits( bool v ) { d_respectLimits = v; }

    enum { DEFMAX_EMIT_PER_STMT=1024, DEFMAX_STMT_PER_SET=2048*64, DEFMAX_EMIT_PER_SET=4*(DEFMAX_STMT_PER_SET) };
    size_t maxEmitCountPerStatement() const { return d_maxEmitCountPerStatement; }
    size_t maxEmitCountPerTrie() const { return d_maxEmitCountPerTrie; }
    size_t maxStatementsPerTrie() const { return d_maxStatementsPerTrie; }

    void set_maxEmitCountPerStatement( size_t x ) { d_maxEmitCountPerStatement=x; }
    void set_maxEmitCountPerTrie(size_t x) { d_maxEmitCountPerTrie=x; }
    void set_maxStatementsPerTrie(size_t x) { d_maxStatementsPerTrie=x; }

	uint8_t getSpellPriority() const { return d_spellPriority; }

	size_t getNumSkippedStatements() const { return numSkippedStatements; }
	void   incrementNumSkippedStatements() { ++numSkippedStatements; }
	void   resetNumSkippedStatements() { numSkippedStatements=0; }

	size_t getNumStatements() const { return numStatements; }
	size_t getNumMacros() const { return numMacros; }
	size_t getNumProcs() const { return numProcs; }

	bool isSilentMode() const { return silentMode; }
	void setSilentMode() { silentMode = true; }

	const std::string& getInputFileName() const { return inputFileName; }

	void setCurTrieId( uint32_t  trieClass, uint32_t trieId )
		{ d_trieIdSet=true; d_curTrieId = trieId; d_curTrieClass = trieClass; }

    void set_noCanonicalNames() { d_noCanonicalNames = 1; }
    void set_canonicalNames() { d_noCanonicalNames = 0; }
    bool is_noCanonicalNames() const { return d_noCanonicalNames; }
	void clearCurTrieId() { d_trieIdSet= false; d_curTrieId=0xffffffff; d_curTrieClass= 0xffffffff; }

	void setTrie( uint32_t trieClass, uint32_t trieId ) ;

	void setTrie(BELTrie *t) { trie = t; }

	BELTrie& getTrie() { return *trie ; }
	const BELTrie& getTrie() const { return *trie ; }
	const BELTrie* getTriePtr() const { return trie ; }
	BELTrie* getTriePtr() { return trie ; }

	GlobalPools& getGlobalPools() { return gp; }
	const GlobalPools& getGlobalPools() const { return gp; }



	BELReader( GlobalPools &g, std::ostream* errStream );

	BELReader( BELTrie* t, GlobalPools &g, std::ostream* errStream );

    std::ostream* getErrStream() { return d_errStream; }


    std::ostream& getErrStreamRef() { return ( d_errStream? *d_errStream: std::cerr); }
	virtual ~BELReader() {
		delete parser;
	}

	void setCurrentUniverse( StoredUniverse* u ) { d_currentUniverse=u; }
    /// initializes current universe to universe for user 0 
    /// this fucntion is ONLY needed for EMITTER. do NOT call it anywhere else
    void initCurrentUniverseToZero();
    //// 
	StoredUniverse* getCurrentUniverse() { return d_currentUniverse; }
	const StoredUniverse* getCurrentUniverse() const { return d_currentUniverse; }

	/// this method is called by the parser for every statement tree
	virtual void addStatement( const BELStatementParsed& );
	virtual void addStatement_imperative( const BELStatementParsed&, bool  );
	virtual void addMacro( uint32_t macroNameId, const BELStatementParsed& );
	/// strId is the id of an internal string representing the procedure name
	virtual void addProc( uint32_t strId, const BELStatementParsed& );

	BELParser*  initParser(InputFormat fmt );
	int  loadFromStream( std::istream& fp );
	//   combines initParser and LoadFromStream
	//   creates the parser and calls loadFromStream
	// sets ruleset spell priority
	int  loadFromFile( const char* fileName, InputFormat fmt = INPUT_FMT_XML );

	std::ostream& printNode( std::ostream& fp, const BarzelTrieNode& node ) const;
    const char* getTrieName() const;
    const char* getTrieClassName() const;
};

inline BELTrie& BELParser::getTrie() { return reader->getTrie(); }
inline const BELTrie& BELParser::getTrie() const { return reader->getTrie(); }

inline GlobalPools& BELParser::getGlobalPools() { return reader->getGlobalPools(); }

inline const GlobalPools& BELParser::getGlobalPools() const { return reader->getGlobalPools(); }



inline std::ostream& BELStatementParsed::getErrStream() const
{
    return( (getReader() && getReader()->getErrStream()) ?
        *(getReader()->getErrStream()) :
        std::cerr );
}

struct BarzXMLErrorStream {
    ay::XMLStream os;
    BarzXMLErrorStream( std::ostream& o ) : os(o) 
        { os.os << "<error>"; }
    BarzXMLErrorStream( std::ostream& o, size_t stmtNum ) : os(o) 
        { os.os << "<error stmt=\""<< stmtNum << "\">"; }
    BarzXMLErrorStream( BELReader& reader, size_t stmtNum ) : 
        os(reader.getErrStreamRef())
    {
        os.os << "<error file=\"" << reader.getInputFileName() << "\" class=\"" << reader.getTrieClassName() << "\" trie=\"" << reader.getTrieName() << "\">";
    }
    ~BarzXMLErrorStream()
        { os.os << "</error>\n"; }
};

} // namespace barzer 
