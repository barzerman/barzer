#ifndef BARZER_EL_TRIE_H
#define BARZER_EL_TRIE_H
#include <barzer_el_btnd.h>
#include <barzer_el_parser.h>
#include <barzer_el_rewriter.h>
#include <ay_bitflags.h>
#include <ay_util.h>
#include <map>

/// data structures representing the Barzer Expression Language BarzEL term pattern trie
/// 
namespace barzer {
struct BELTrie;
class BarzelRewriterPool;
class BarzelWildcardPool;
struct BELPrintContext;
struct BELTrieContext;
struct BELPrintFormat;

/// this type is used as a key by firmchild lookup (BarzelFCLookup)
struct BarzelTrieFirmChildKey {
	uint32_t id;
	uint8_t  type;
	// when noLeftBlanks is !=0 
	// the firm child will only match if it immediately follows the prior token 
	// in other words spaces wont be skipped (!)
	// tis is needed for patterns defining dates and such
	uint8_t  noLeftBlanks;
	
	BarzelTrieFirmChildKey() : id(0xffffffff), type(BTND_Pattern_None_TYPE), noLeftBlanks(0) {}

	BarzelTrieFirmChildKey(const BTND_Pattern_Token& x) : id(x.stringId), type((uint8_t)BTND_Pattern_Token_TYPE), noLeftBlanks(0) {}
	BarzelTrieFirmChildKey(const BTND_Pattern_Punct& x) : id(x.theChar),  type((uint8_t)BTND_Pattern_Punct_TYPE), noLeftBlanks(0) {}
	BarzelTrieFirmChildKey(const BTND_Pattern_CompoundedWord& x) : 
		id(x.compWordId), type((uint8_t)BTND_Pattern_CompoundedWord_TYPE), noLeftBlanks(0)
	{}
	
	inline BarzelTrieFirmChildKey( const BTND_PatternData& p ) 
	{
		noLeftBlanks=0;
		switch( p.which() ) {
		case BTND_Pattern_StopToken_TYPE:
			type = (uint8_t)BTND_Pattern_StopToken_TYPE;
			id = boost::get<BTND_Pattern_StopToken>( p ).stringId;	
			break;
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
	inline BarzelTrieFirmChildKey& set( const BarzerLiteral& dta, bool followsBlank=false )
	{
		noLeftBlanks = ( followsBlank ? 0:1 );
		id =   dta.getId();
		switch(dta.getType()) {
		case BarzerLiteral::T_STRING:   type=(uint8_t) BTND_Pattern_Token_TYPE; break;
		case BarzerLiteral::T_COMPOUND: type=(uint8_t) BTND_Pattern_CompoundedWord_TYPE; break;
		case BarzerLiteral::T_STOP:     type=(uint8_t) BTND_Pattern_StopToken_TYPE; break;
		case BarzerLiteral::T_PUNCT:    type=(uint8_t) BTND_Pattern_Punct_TYPE; break;
		case BarzerLiteral::T_BLANK: 
			type = (typeof type) BTND_Pattern_Token_TYPE;
			id =   0xffffffff;
			break;
		}
		return *this;
	}
	void setNull( ) { type = BTND_Pattern_None_TYPE; id = 0xffffffff; noLeftBlanks=0;}

	std::ostream& print( std::ostream& , const BELPrintContext& ctxt ) const;
	
	std::ostream& print( std::ostream& ) const;
	bool isNull() const { return (type == BTND_Pattern_None_TYPE); }

	bool isBlankLiteral() const { return (BTND_Pattern_Token_TYPE ==type && id==0xffffffff); } 
	bool isStopToken() const { return( BTND_Pattern_StopToken_TYPE == type ); }

	bool isNoLeftBlanks() const { return noLeftBlanks; }
};

inline std::ostream& operator <<( std::ostream& fp, const BarzelTrieFirmChildKey& key )
{ return key.print(fp); }

struct BarzelTrieFirmChildKey_comp_less {
	inline bool operator() ( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r ) const 
		{ 
			return ay::range_comp().less_than( 
				l.type, l.id, l.noLeftBlanks,
				r.type, r.id, r.noLeftBlanks
			);
			// return( l.id < r.id ? true : ( r.id < l.id ? false : (l.type < r.type))); 
		}
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
	uint32_t wcId; // wildcard id unique for the type in a pool
	uint8_t wcType; // one of BTND_Pattern_XXXX_TYPE enums
	// longest span of the wildcard
	// for example if number wildcard has maxSpan of 1 
	// only 1 bead long sequences will be considered candidates
	// for matching
	uint8_t maxSpan; 
	uint8_t noLeftBlanks; // one of BTND_Pattern_XXXX_TYPE enums
	
	void clear() 
		{ wcType = BTND_Pattern_None_TYPE; wcId= 0xffffffff; }
	bool isBlank() const 
		{ return( wcType == BTND_Pattern_None_TYPE ); }
	BarzelWCKey() : 
		wcId(0xffffffff), 
		wcType(BTND_Pattern_None_TYPE),
		maxSpan(1),
		noLeftBlanks(0)
	{}
	void set( uint8_t span, bool followsBlank ) 
	{
		maxSpan= span;
		noLeftBlanks = ( followsBlank? 0: 1 );
	}

	// non trivial constructor - this may add wildcard to the pool
	// when fails wcType will be set to BTND_Pattern_None_TYPE
	// BarzelWCKey( BELTrie& trie, const BTND_PatternData& pat );

	inline bool lessThan( const BarzelWCKey& r ) const
	{ 
		if( 
			ay::range_comp().less_than( 
				maxSpan, 	wcType,
				r.maxSpan, 	r.wcType
			) 
		) {
			return true;
		} else if( 
			ay::range_comp().less_than( 
				r.maxSpan, 	r.wcType,
				maxSpan, 	wcType
			) 
		) {
			return false;
		} else
			return ( ay::range_comp().less_than(
				r.wcId, r.noLeftBlanks,
				  wcId,   noLeftBlanks
			) );
	}
	
	std::ostream& print( std::ostream& fp, const BarzelWildcardPool* ) const;
};


inline bool operator <( const BarzelWCKey& l, const BarzelWCKey& r )
{
	return l.lessThan( r );
}


/// barzel wildcard child lookup object 
/// stored in barzel trie nodes

typedef std::pair<BarzelTrieFirmChildKey, BarzelWCKey> BarzelWCLookupKey;
inline bool operator< (const BarzelWCLookupKey& l, const BarzelWCLookupKey& r )
{
	return ay::range_comp().less_than(
		l.first.id, l.first.type, l.second, l.first.noLeftBlanks,
		r.first.id, r.first.type, r.second, r.first.noLeftBlanks
	);
}

/// right side of the pattern 
class BarzelTranslation {
public:
	typedef enum {
		T_NONE,   // blank translation 
		T_STOP, // translates into stop token 
		T_STRING, // dynamic string or literal
		T_COMPWORD, // compounded word
		T_NUMBER_INT, // integer 
		T_NUMBER_REAL, // real number (float)
		/// this one is very special - this actually creates a tree 
		T_REWRITER, // interpreted sequence

		T_BLANK,    // blank literal
		T_PUNCT,    // punctuation
		T_VAR_NAME, // variable name rewrite
		T_VAR_WC_NUM, // variable rewrite on wildcard number 
		T_VAR_EL_NUM, // variable rewrite on raw element number (PN)
		T_VAR_GN_NUM, // variable rewrite on raw element number

		T_MAX
	} Type_t;
	boost::variant< uint32_t, double, int> id;
	uint8_t  type; // one of T_XXX constants 
		
	void set( BELTrie& trie, const BTND_Rewrite_Literal& );
	void set( BELTrie& trie, const BTND_Rewrite_Number& );
	void set( BELTrie& trie, const BTND_Rewrite_Variable& );

	void set( BELTrie& trie, const BELParseTreeNode& );

	void setStop() { type = T_STOP; }

	//// this is invoked from el_parser
	template <typename T>
	inline void setBtnd( BELTrie& trie, const T& x ) {
		AYDEBUG("unsupported trivial rewrite" );
		setStop();
	}

	void setRewriter( uint32_t rid ) 
	{
		type = T_REWRITER;
		id = rid;
	}

	uint32_t getRewriterId() const
	{
		return ( type == T_REWRITER ? boost::get<uint32_t>(id) : 0xffffffff );
	}
	uint32_t getId_uint32() const { return ( id.which() ==0 ? boost::get<uint32_t>(id) : 0xffffffff ); }
	int getId_int() const { return ( id.which() ==2  ? boost::get<int>(id) : 0 ); }
	double getId_double() const { return ( id.which() ==1  ? boost::get<double>(id) : 0.0 ); }


	void clear() { type= T_NONE; }
	bool isRewriter() const  { return ( type == T_REWRITER ); }
		 
	// returns if evaluation may theoretically fail - theoretically some rewrites may 
	// contain things such as entity searches or other operations resulting in entity lists
	// as well as potentially fail conditions - post check may be more complicated than the 
	// initial wildcard matching check
	// this function answers in principle whether the right side may theoretically fail 
	// this check is very cheap because no actual evaluation is performed
	bool isFallible(const BarzelRewriterPool& rwrPool) const 
		{ return( type != T_REWRITER ? false : isRewriteFallible( rwrPool ) ); }

	/// resolves 
	void fillRewriteData( BTND_RewriteData& ) const;

	bool isRewriteFallible( const BarzelRewriterPool& pool ) const;
	bool nonEmpty() const
		{ return ( type != T_NONE ); }
	BarzelTranslation() : id( 0xffffffff ) , type(T_NONE) {}
	BarzelTranslation(Type_t t , uint32_t i ) : id(i),type((uint8_t)t) {}
	
	std::ostream& print( std::ostream& , const BELPrintContext& ) const;

};
template <>
inline void BarzelTranslation::setBtnd<BTND_Rewrite_Literal>( BELTrie& trie, const BTND_Rewrite_Literal& x ) { set(trie,x); }
template <>
inline void BarzelTranslation::setBtnd<BTND_Rewrite_Number>( BELTrie& trie, const BTND_Rewrite_Number& x ) { set(trie,x); }
template <>
inline void BarzelTranslation::setBtnd<BTND_Rewrite_Variable>( BELTrie& trie, const BTND_Rewrite_Variable& x ) { set(trie,x); }

class BarzelTrieNode;
typedef std::map<BarzelTrieFirmChildKey, BarzelTrieNode > BarzelFCMap;

class BarzelTrieNode {
	BarzelFCMap firmMap; /// children of the 'firm' types - token,punctuation,compounded word

	uint32_t wcLookupId; // when valid (not 0xffffffff) can it's an id of a wildcard lookup object 
	// BarzelWCLookup wcChild; /// used for wildcard matching (number,date, etc)
	enum {
		B_WCCHILD, // it's a wildcard child of its parent 
		
		// new bits strictly above this line 
		B_MAX
	};

	ay::bitflags<B_MAX> d_flags; // only prints children at the current level

	const BarzelTrieNode* d_parent;
public:
	BarzelTranslation translation;

	BarzelTrieNode():
		wcLookupId(0xffffffff) ,
		d_parent(0)
	{}
	BarzelTrieNode(const BarzelTrieNode* p ):
		wcLookupId(0xffffffff) ,
		d_parent(p)
	{}

	const BarzelTrieNode* getParent() const { return d_parent; }
	uint32_t getWCLookupId() const { return wcLookupId; }

	void clear();
	std::ostream& print_firmChildren( std::ostream& fp, BELPrintContext& ) const;
	std::ostream& print_wcChildren( std::ostream& fp, BELPrintContext& ) const;
	std::ostream& print_translation( std::ostream& fp, const BELPrintContext& ) const;

	bool hasFirmChildren() const { return (!firmMap.empty()); }
	BarzelFCMap& getFirmMap() { return firmMap; }
	const BarzelFCMap& getFirmMap() const { return firmMap; }

	bool hasWildcardChildren() const { return (wcLookupId != 0xffffffff ) ; }
	bool hasChildren() const 
		{ return (hasWildcardChildren() || hasFirmChildren() ); }

	bool isLeaf() const { return translation.nonEmpty(); }

	/// makes node leaf and sets translation
	void setTranslation(BELTrie&trie, const BELParseTreeNode& ptn );

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

	void setFlag_isWcChild() { d_flags.set( B_WCCHILD ); }
	bool isWcChild() const { return d_flags[ B_WCCHILD ]; }

	const BarzelTrieNode* getFirmChild( const BarzelTrieFirmChildKey& key ) const
	{
		BarzelFCMap::const_iterator  i = firmMap.find( key );
		return ( i == firmMap.end() ? 0 : &(i->second) );
	}

	std::ostream& print( std::ostream& , BELPrintContext& ) const;
};

typedef std::map< BarzelWCLookupKey, BarzelTrieNode > BarzelWCLookup;

struct BELTrie {
	BarzelRewriterPool* rewrPool;
	BarzelWildcardPool* wcPool;

	BarzelTrieNode root;
	BELTrie( BarzelRewriterPool*  rPool, BarzelWildcardPool* wPool ) : rewrPool(rPool), wcPool(wPool) {}

	/// stores wildcard data n a form later usable by the Trie
	/// this ends up calling wcPool->produceWCKey()
	void produceWCKey( BarzelWCKey&, const BTND_PatternData&   );

	/// adds a new path to the trie
	// returns the leaf. if this function returns null it would indicate some major loading inconsistency
	const BarzelTrieNode* addPath( const BTND_PatternDataVec& path, const BELParseTreeNode& trans );

	BarzelTrieNode& getRoot() { return root; }
	const BarzelTrieNode& getRoot() const { return root; }

	/// print methods 
	std::ostream& print( std::ostream&, BELPrintContext& ctxt ) const;

	void clear();
};

/// object necessary for meaningful printing
struct BELPrintFormat {
	enum {
		PFB_NODESCEND, // doesnt recursively print the children 

		/// add new flags only above this line
		PFB_MAX
	};
	ay::bitflags<PFB_MAX> flags; // only prints children at the current level

	/// PFB_NODESCEND flag
	bool doDescend() const 	{ return (!flags[PFB_NODESCEND]); }
	void setNodescend() 	{ flags.set( PFB_NODESCEND ); }

};
struct BELTrieContext {
	BELTrie& trie;
	const ay::UniqueCharPool& strPool;

	BELTrieContext(
		BELTrie& t, 
		const ay::UniqueCharPool& sp 
	) : 
		trie(t), 
		strPool(sp)
	{}
};

struct BELPrintContext {
	const BELTrie& trie;
	const ay::UniqueCharPool& strPool;
	const BELPrintFormat& format;

	std::string prefix;
	int depth;
	enum { PREFIX_INDENT_SZ = 4 };	

	void descend() { ++depth; prefix.resize( prefix.size() + PREFIX_INDENT_SZ, ' ' ); }
	void ascend() { 
		if( depth > 0 ) 
			--depth;
		if( prefix.size() >= PREFIX_INDENT_SZ )
			prefix.resize( prefix.size() - PREFIX_INDENT_SZ ); 
	}
	BELPrintContext( 
		const BELTrie& t, 
		const ay::UniqueCharPool& sp ,
		const BELPrintFormat& f
	) : 
		trie(t), 
		strPool(sp),
		format(f),
		depth(0)
	{}
	

	const char* printableString( uint32_t id )  const
	{ return strPool.printableStr(id); }
	
	const BarzelWCLookup*  getWildcardLookup( uint32_t id ) const; 

	bool needDescend() const
	{
		return( !depth || format.doDescend() );
	}

	std::ostream& printBarzelWCLookupKey( std::ostream& fp, const BarzelWCLookupKey& key ) const;

	// std::ostream& printRewriterByteCode( std::ostream& fp, const BarzelRewriterPool::BufAndSize& );
	std::ostream& printRewriterByteCode( std::ostream& fp, const BarzelTranslation& ) const;
};

}

#endif // BARZER_EL_TRIE_H

