#ifndef BARZER_EL_TRIE_H
#define BARZER_EL_TRIE_H
#include <barzer_el_btnd.h>
#include <map>

/// data structures representing the Barzer Expression Language BarzEL term pattern trie
/// 
namespace barzer {

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

/// barzel wildcard child lookup object 
/// stored in barzel trie nodes
struct BarzelWCCLookup {
};


class BarzelTrieNode {
	enum {
		BIT_LEAF,

		BIT_MAX
	};
	ay::bitflags<BIT_MAX> nodeBits;

	typedef std::map<BarzelTrieFirmChildKey, BarzelTrieNode > BarzelFCMap; 
	BarzelFCMap firmMap; /// children of the 'firm' types - token,punctuation,compounded word

	BarzelWCCLookup wcChild; /// used for wildcard matching (number,date, etc)
};

}

#endif // BARZER_EL_TRIE_H

