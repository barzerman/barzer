#ifndef BARZER_STORAGE_TYPES_H
#define BARZER_STORAGE_TYPES_H

#include <barzer_token.h>
#include <vector>
#include <map>
#include <ay/ay_bitflags.h>

/// Storage types are the types used in read-only data structures to store 
/// tokens (both compounded and single) and Entities
namespace barzer {

class StoredEntity;
class StoredToken;

typedef int32_t StoredTokenId; 
typedef int32_t StoredEntityId; 
const int32_t INVALID_STORED_ID = -1;

//// stores information relevant to a relationship between one token and one entity
//// StoredToken has an array of pairs (TokenEntityLinkInfo,StoredEntity) 
class TokenEntityLinkInfo {
	int8_t strength; // 0 - default regular match, negative - depression, positive - boost

	// degree to which this token alone is allowed to match on the entity
	// -100,100 ~ the lower the number the less likely it will be to match the entity by this 
	// token alone
	int8_t indicative; 
	enum {
		TELI_BIT_STEM, // only associated as stem
		/// OCC_BOOST1/2 fields together represent the boost from multiple occurences of the same token 
		/// in the entity . they are together a 2bit number.
		/// 00 means there will be no extra premium for more than 1 occurence 
		/// 01 - 2, 10 - 3, 11 - 4 or more 
		TELI_BIT_OCC_BOOST1, 
		TELI_BIT_OCC_BOOST2,

		TELI_BIT_MAX
	};
	ay::bitflags<TELI_BIT_MAX> bits;

public:
	bool    isStem( )   const { return bits[ TELI_BIT_STEM ] ; }
	bool    isMultiOcc()const { return (bits[TELI_BIT_OCC_BOOST1] || bits[TELI_BIT_OCC_BOOST2] ); }
	uint8_t getNumOcc() const { return(1+(bits[TELI_BIT_OCC_BOOST1] ? 1:0) + (bits[TELI_BIT_OCC_BOOST2] ? 2:0 ));}

	// sets the stem flag 
		   void setStem() { bits.set(TELI_BIT_STEM); }
	// sets maximum boostin number of occurences
	inline void setNumOcc(int n) { 
		if( n==2) { bits.set(TELI_BIT_OCC_BOOST1); bits.set(TELI_BIT_OCC_BOOST2,0); } 
		else if( n==1) { bits.unset(TELI_BIT_OCC_BOOST1); bits.unset(TELI_BIT_OCC_BOOST2); } 
		else if( n==3) { bits.unset(TELI_BIT_OCC_BOOST1); bits.set(TELI_BIT_OCC_BOOST2); } 
		else { bits.set(TELI_BIT_OCC_BOOST1); bits.set(TELI_BIT_OCC_BOOST2); } 
	}
	int8_t getStrength() const { return strength; }
	int8_t getIndicative() const { return indicative; }

	void setStrength(int8_t s)   { strength=s; }
	void setIndicative(int8_t i) { indicative=i; }

	TokenEntityLinkInfo( ) : strength(0), indicative(0) {}
	TokenEntityLinkInfo( int8_t str, int8_t i=0) : strength(str), indicative(i) {}
};


//// pair Entity Id,TokenEntityLinkInfo 
typedef std::pair<StoredEntityId, TokenEntityLinkInfo > SETELI_pair;

typedef std::vector<SETELI_pair> SETELI_pair_vec;

/// StoredToken represents a single token (simple or compounded) 
/// Every StoredToken will have a unique id issued by DataIndex
struct StoredToken {
	StoredTokenId tokId;
	StoredTokenClassInfo classInfo;
	uint16_t numWords; // number of words in the token 1 or more for compounded
	uint16_t length;   // full length of all participatin tokens and spaces

	SETELI_pair_vec entVec;

	StoredToken( ) : tokId(INVALID_STORED_ID), numWords(0), length(0) {}
};


/// information stored together with a token id within a "name" 
/// each Entity may have a few privileged token sequences which are 
/// treated as names. This is the way to take the order of tokens 
/// into account. 
/// at first everything will just have seqId 0 which means an unordered 
/// assortment of linked tokens
struct EntTokenOrderInfo {
	uint16_t seqId; // sequence id. 0 means an unordered collection of tokens 
	uint8_t  idx;   // index within sequence. for seqId this value is irrelevant
	// bitflags
	enum {
		ETOIBIT_ID
	};
	ay::bitflags<8> bits;   // index within sequence. for seqId this value is irrelevant
}; 
typedef std::pair< StoredTokenId, EntTokenOrderInfo > StoredTokenSeqInfo_pair;
/// Stored Entity - thes objects live in DataIndex - they're only modified on load

/// represents Token,EntityClass,EntitySubclass - unique id
struct UniqEntityTok {
	StoredTokenId tokId;
	uint16_t eClass, eSubClass;
	UniqEntityTok( ) : 
		tokId(INVALID_STORED_ID),
		eClass(0),
		eSubClass(0)
	{}
	UniqEntityTok( StoredTokenId t, uint16_t ec, uint16_t esc ) : 
		tokId(t),
		eClass(ec),
		eSubClass(esc)
	{}
};

struct StoredEntity {
	StoredEntityId entId; // entity id unique across all classes

	int32_t relevance; // 0 - 2,000,000,000
	typedef std::vector<StoredTokenSeqInfo_pair> STSI_vec; 

	StoredEntity() : entId(INVALID_STORED_ID), relevance(0) {}
};


}
#endif // BARZER_STORAGE_TYPES_H
