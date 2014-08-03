#pragma once 
#include <barzer_basic_types.h>
namespace barzer {

/// right side of the pattern
class BarzelTranslation {
public:
    uint32_t tranId; // id of this translation as it's known to the owner trie
    
	typedef enum {
		T_NONE,   // blank translation
		T_STOP, // translates into stop token
		T_STRING, // dynamic string or literal
		T_COMPWORD, // compounded word
		T_NUMBER_INT, // integer
		T_NUMBER_REAL, // real number (float)
        
		/// this one is very special - this actually creates a tree
		T_RAWTREE, // BELParseTreeNode as is (faster to compute)   

		T_BLANK,    // blank literal
		T_PUNCT,    // punctuation
		T_VAR_NAME, // variable name rewrite
		T_VAR_WC_NUM, // variable rewrite on wildcard number
		T_VAR_EL_NUM, // variable rewrite on raw element number (PN)
		T_VAR_GN_NUM, // variable rewrite on raw element number
		T_MKENT, // entity maker
		T_MKENTLIST, // entity list maker
		T_FUNCTION, // fucntion(void)
        T_REQUEST_VAR_NAME, // request wide variable MODE_REQUEST_VAR (RequestEnvironment::d_reqVar)

		T_MAX
	} Type_t;
	boost::variant< uint32_t, double, int64_t> id;
	BarzelTranslationTraceInfo traceInfo;
	uint8_t  type; // one of T_XXX constants
	uint8_t  makeUnmatchable; // when !=0 this is a terminating translation ... the value will
                                // be assigned to the unmatchability of the affected beads

    uint32_t argStrId;
    int8_t  confidenceBoost; // can be between -CONFIDENCE_HIGH and +CONFIDENCE_HIGH 

	void set( BELTrie& trie, const BTND_Rewrite_Literal& );
	void set( BELTrie& trie, const BTND_Rewrite_Number& );
	void set( BELTrie& trie, const BTND_Rewrite_MkEnt& );
	void set( BELTrie& trie, const BTND_Rewrite_Variable& );
	void set( BELTrie& trie, const BTND_Rewrite_Function& );
	void setMkEntList( uint32_t i )
	{
		type = T_MKENTLIST;
		id = i;
	}

	enum {
		ENCOD_FAILED = -1,
		ENCOD_RAWTREE=0,
		ENCOD_TRIVIAL
	};
	int set( BELTrie& trie, const BELParseTreeNode& );

	void setStop() { type = T_STOP; }

	//// this is invoked from el_parser
	template <typename T>
	inline int setBtnd( BELTrie& trie, const T& x ) {
		// AYDEBUG("unsupported trivial rewrite" );
		setStop();
        return 1;
	}

	void setRawTree( uint32_t rid )
	{
		type = T_RAWTREE;
		id = rid;
	}

	uint8_t getType() const { return type; }
	uint32_t getRewriterId() const
        { return ( (type==T_RAWTREE) ? boost::get<uint32_t>(id) : 0xffffffff ); }

	uint32_t getId_uint32() const { return ( id.which() ==0 ? boost::get<uint32_t>(id) : 0xffffffff ); }
	int64_t getId_int() const { return ( id.which() ==2  ? boost::get<int64_t>(id) : 0 ); }
	double getId_double() const { return ( id.which() ==1  ? boost::get<double>(id) : 0.0 ); }
    uint32_t getArgStrId() const { return argStrId; }


	void clear() { type= T_NONE; }
	bool isMkEntSingle() const  { return ( type == T_MKENT ); }
	bool isMkEntList() const  { return ( type == T_MKENTLIST ); }
    bool isEntity() const { return ( isMkEntSingle() || isMkEntList() ); }
	bool isRawTree() const  { return ( type == T_RAWTREE ); }

	/// resolves
	void fillRewriteData( BTND_RewriteData& ) const;

	bool nonEmpty() const
		{ return ( type != T_NONE ); }
	BarzelTranslation() : tranId(0xffffffff), id( 0xffffffff ) , type(T_NONE), makeUnmatchable(0), argStrId(0xffffffff), confidenceBoost(0) {}
	// BarzelTranslation(Type_t t , uint32_t i ) : id(i),type((uint8_t)t), makeUnmatchable(0), argStrId(0xffffffff) {}

	std::ostream& print( std::ostream& , const BELPrintContext& ) const;
    
    uint32_t getId() const { return tranId; }
};
template <>
inline int BarzelTranslation::setBtnd<BTND_Rewrite_Function>( BELTrie& trie, const BTND_Rewrite_Function& x ) {
	set(trie,x);
    return 0;
}
template <> inline int BarzelTranslation::setBtnd<BTND_Rewrite_Literal>( BELTrie& trie, const BTND_Rewrite_Literal& x ) { set(trie,x); return 0;}
template <> inline int BarzelTranslation::setBtnd<BTND_Rewrite_Number>( BELTrie& trie, const BTND_Rewrite_Number& x ) { set(trie,x); return 0; }
template <> inline int BarzelTranslation::setBtnd<BTND_Rewrite_Variable>( BELTrie& trie, const BTND_Rewrite_Variable& x ) { set(trie,x); return 0; }
template <> inline int BarzelTranslation::setBtnd<BTND_Rewrite_MkEnt>( BELTrie& trie, const BTND_Rewrite_MkEnt& x ) { set(trie,x); return 0; }

/// AmbiguousTraceXXX is needed for rules deletion in cases when the same pattern in different statements maps 
/// into different translations. This type of ambiguity is resolved in one of the two ways:
///  - entity list is formed for simple mkent translations (TYPE_ENTITY)
///  - extra nodes are added for all other kinds (especially mkERC) (TYPE_SUBTREE)
/// for both ways 32 bit unsigned ingteger ids are used

class AmbiguousTraceInfo : public AmbiguousTraceId {
public:
    uint32_t refCount;
public:
    AmbiguousTraceInfo( const AmbiguousTraceId& i ) : AmbiguousTraceId(i), refCount(0) {}
    AmbiguousTraceInfo( ) : refCount(0) {}
    AmbiguousTraceInfo( uint32_t i ) : AmbiguousTraceId(i), refCount(0) {}

    AmbiguousTraceInfo& setEntity( uint32_t i ) { return ( AmbiguousTraceId::setEntity(i), *this ); }
    AmbiguousTraceInfo& setSubtree( uint32_t  i ) { return ( AmbiguousTraceId::setSubtree(i), *this ); }

    AmbiguousTraceInfo& lock() { ++id; return *this; }
    // when returns true no references left
    AmbiguousTraceInfo& unlock() {
        if( id ) --id;
        return *this;
    }
    uint32_t getRefCount() const { return refCount; }
    bool equal( const AmbiguousTraceId& o ) const
        { return ( o.type == type && o.id == id ); }
};

} // namespace barzer 
