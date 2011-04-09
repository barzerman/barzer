#ifndef BARZER_EL_TRIE_H
#define BARZER_EL_TRIE_H
#include <barzer_el_btnd.h>
#include <barzer_el_parser.h>
#include <barzer_el_rewriter.h>
#include <map>

/// data structures representing the Barzer Expression Language BarzEL term pattern trie
/// 
namespace barzer {
struct BELTrie;
class BarzelRewriterPool;
class BarzelWildcardPool;

/// this type is used as a key by firmchild lookup (BarzelFCLookup)
struct BarzelTrieFirmChildKey {
	uint32_t id;
	uint8_t  type;
	
	BarzelTrieFirmChildKey() : id(0xffffffff), type(BTND_Pattern_None_TYPE) {}

	BarzelTrieFirmChildKey(const BTND_Pattern_Token& x) : id(x.stringId), type((uint8_t)BTND_Pattern_Token_TYPE) {}
	BarzelTrieFirmChildKey(const BTND_Pattern_Punct& x) : id(x.theChar),  type((uint8_t)BTND_Pattern_Punct_TYPE) {}
	BarzelTrieFirmChildKey(const BTND_Pattern_CompoundedWord& x) : 
		id(x.compWordId), type((uint8_t)BTND_Pattern_CompoundedWord_TYPE){}
	
	inline BarzelTrieFirmChildKey( const BTND_PatternData& p ) 
	{
		switch( p.which() ) {
		case BTND_Pattern_Token_TYPE:
			type = (uint8_t)BTND_Pattern_Token_TYPE;
			id = boost::get<BTND_Pattern_Token>( p ).stringId;	
			break;
		case BTND_Pattern_Punct_TYPE:
			type = (uint8_t) BTND_Pattern_Punct_TYPE;
			id = boost::get<BTND_Pattern_Punct>( p ).theChar;
			break;
		case BTND_Pattern_CompoundedWord_TYPE:
			type = (uint8_t) BTND_Pattern_CompoundedWord_TYPE;
			id = boost::get<BTND_Pattern_CompoundedWord>( p ).compWordId;
			break;
		default:
			type=BTND_Pattern_None_TYPE; // isNone will return true for this
			id=0xffffffff;
		}
	}
	std::ostream& print( std::ostream& ) const;
	bool isBlank() const { return (type == BTND_Pattern_None_TYPE); }
};

inline std::ostream& operator <<( std::ostream& fp, const BarzelTrieFirmChildKey& key )
{ return key.print(fp); }

struct BarzelTrieFirmChildKey_comp_less {
	inline bool operator() ( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r ) const 
		{ return( l.id < r.id ? true : ( r.id < l.id ? false : (l.type < r.type))); }
};
inline bool operator <( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r ) 
{ return BarzelTrieFirmChildKey_comp_less() ( l, r ); }

struct BarzelTrieFirmChildKey_comp_eq {
	inline bool operator() ( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r ) const
		{ return ( l.id == r.id && l.type == r.type );}
};
inline bool operator ==( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r )
{ return BarzelTrieFirmChildKey_comp_eq() ( l, r ); }

/// wildcard child key
struct BarzelWCKey {
	BarzelTrieFirmChildKey	firmFollowerKey; // first firm key following this wildcard
	
	uint32_t wcId; // wildcard id unique for the type in a pool
	uint8_t wcType; // one of BTND_Pattern_XXXX_TYPE enums
	
	bool clear() 
		{ wcType = BTND_Pattern_None_TYPE; wcId= 0xffffffff; }
	bool isBlank() const 
		{ return( wcType == BTND_Pattern_None_TYPE ); }
	BarzelWCKey() : wcId(0xffffffff), wcType(BTND_Pattern_None_TYPE) {}

	// non trivial constructor - this may add wildcard to the pool
	// when fails wcType will be set to BTND_Pattern_None_TYPE
	BarzelWCKey( BELTrie& trie, const BTND_PatternData& pat );

	inline bool lessThan( const BarzelWCKey& r ) const
	{ return ay::range_comp()::less_than( wcType, wcId, r.wcType, r.wcId ); }
};
inline bool operator <( const BarzelWCKey& l, const BarzelWCKey& r )
{
	return l.lessThan( r );
}

/// barzel wildcard child lookup object 
/// stored in barzel trie nodes

typedef std::pair<BarzelTrieFirmChildKey, BarzelWCKey> BarzelWCLookupKey;

typedef std::map< BarzelWCLookupKey, BarzelTrieNode > BarzelWCLookup;

/// right side of the pattern 
class BarzelTranslation {
public:
	typedef enum {
		T_NONE,   // blank translation 
		T_STOP, // translates into stop token 
		T_STRING, // string literal
		T_COMPWORD, // compounded word
		T_NUMBER_INT, // integer 
		T_NUMBER_REAL, // real number (float)
		T_REWRITER, // interpreted sequence
		
		T_MAX
	} Type_t;
	boost::variant< uint32_t, double> id;
	uint8_t  type; // one of T_XXX constants 
		
	void set( BELTrie& trie, const BTND_Rewrite_Literal& );
	void set( BELTrie& trie, const BTND_Rewrite_Number& );

	void set( BELTrie& trie, const BELParseTreeNode& );

	void setRewriter( uint32_t rid ) 
	{
		type = T_REWRITER;
		id = rid;
	}
	void setStop() { type = T_STOP; }

	void clear() { type= T_NONE; }
	bool nonEmpty() const
		{ return ( type != T_NONE ); }
	BarzelTranslation() : id( 0xffffffff ) , type(T_NONE) {}
	BarzelTranslation(Type_t t , uint32_t i ) : id(i),type((uint8_t)t) {}
};

class BarzelTrieNode {
	typedef std::map<BarzelTrieFirmChildKey, BarzelTrieNode > BarzelFCMap; 
	BarzelFCMap firmMap; /// children of the 'firm' types - token,punctuation,compounded word

	uint32_t wcLookupId; // when valid (not 0xffffffff) can it's an id of a wildcard lookup object 
	// BarzelWCLookup wcChild; /// used for wildcard matching (number,date, etc)
public:
	BarzelTranslation translation;

	bool isLeaf() { return translation.nonEmpty(); }
	/// makes node leaf and sets translation
	void setTranslation(BELTrie&trie, const BELParseTreeNode& ptn ) { translation.set(trie, ptn); }

	// locates a child node or creates a new one and returns a reference to it. non-leaf by default 
	// if pattern data cant be translated into a valid key the same node is returned
	/// so typically  when we want to add a chain of patterns as a path to the trie 
	/// we will iterate over the chain calling node = node->addPattern(p) for each pattern 
	/// and in the end we will call node->setTranslation() 

	// if p is a non-firm kind (a wildcard) this is a no-op
	BarzelTrieNode* addFirmPattern( BELTrie& trie, const BTND_PatternData& p );

	// if p is firm (not a wildcard) this is a no-op
	BarzelTrieNode* addWildcardPattern( BELTrie& trie, const BTND_PatternData& p, const BarzelTrieFirmChildKey& fk  );

	bool hasValidWcLookup() const
		{ return (wcLookupId != 0xffffffff); }
	BarzelTrieNode() : wcLookupId(0xffffffff) {}
};

struct BELTrie {
	BarzelRewriterPool* rewrPool;
	BarzelWildcardPool* wcPool;

	BarzelTrieNode root;
	BELTrie( BarzelRewriterPool*  rPool, BarzelWildcardPool* wPool ) : rewrPool(pool), wcPool(wPool) {}

	/// stores wildcard data n a form later usable by the Trie
	/// this ends up calling wcPool->produceWCKey()
	void produceWCKey( BarzelWCKey&, const BTND_PatternData&   );

	/// adds a new path to the trie
	void addPath( const BTND_PatternDataVec& path, const BELParseTreeNode& trans );
};

}

#endif // BARZER_EL_TRIE_H

