#ifndef BARZER_STORAGE_TYPES_H
#define BARZER_STORAGE_TYPES_H

#include <ay/ay_headers.h>
#include <barzer_token.h>
#include <vector>
#include <algorithm>
#include <map>
#include <ay/ay_util.h>
#include <ay/ay_bitflags.h>
#include <ay/ay_string_pool.h>
#include <barzer_entity.h>

/// Storage types are the types used in read-only data structures to store 
/// tokens (both compounded and single) and Entities
namespace barzer {

struct StoredEntity;
struct StoredToken;

typedef uint32_t StoredTokenId; 

typedef uint32_t StoredEntityId; 
const uint32_t INVALID_STORED_ID = 0xffffffff;

//// stores information relevant to a relationship between one token and one entity
//// StoredToken has an array of pairs (TokenEntityLinkInfo,StoredEntity) 
struct TokenEntityLinkInfo {
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
		TELI_BIT_UNIQID, // token is a unique id for the entity
		TELI_BIT_MISSPELL, // only associated as known misspelling
		TELI_BIT_EUID, // this token is a euid

		TELI_BIT_MAX
	};
	ay::bitflags<TELI_BIT_MAX> bits;

	static int8_t getValidInt8( int x ) 
		{ return ( x>=-100 && x<= 100 ? (int8_t)x: 0); }
	bool    isStem( )   const { return bits[ TELI_BIT_STEM ] ; }
	bool    isMultiOcc()const { return (bits[TELI_BIT_OCC_BOOST1] || bits[TELI_BIT_OCC_BOOST2] ); }
	uint8_t getNumOcc() const { return(1+(bits[TELI_BIT_OCC_BOOST1] ? 1:0) + (bits[TELI_BIT_OCC_BOOST2] ? 2:0 ));}

	// sets the stem flag 
		   void setBit_Stem() { bits.set(TELI_BIT_STEM); }
		   void setBit_Uniqid() { bits.set(TELI_BIT_UNIQID); }
		   void setBit_Misspell() { bits.set(TELI_BIT_MISSPELL); }
		   void setBit_Euid() { bits.set(TELI_BIT_EUID); }

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

	// when numWords =1 this is resolved from the pool 
	// otherwise it's resolved from the compounded names object 
	ay::UniqueCharPool::StrId stringId; 

	uint16_t numWords; // number of words in the token 1 or more for compounded
	uint16_t length;   // full length of all participatin tokens and spaces

	StoredTokenClassInfo classInfo;

	// SETELI_pair_vec entVec;
	
	bool isSimpleTok() const { return (classInfo.isSimple() && numWords == 1); }
	bool isCompoundedTok() const { return (numWords> 1 || classInfo.isCompounded()); }

	inline void setSingle( 
		StoredTokenId tid, 
		ay::UniqueCharPool::StrId sid, 
		uint16_t len ) 
	{
		tokId= tid;
		stringId = sid;
		numWords = 1;
		length = len;
	}

	void addEntity( StoredEntityId /*entId*/, const TokenEntityLinkInfo& /*teli*/ )
	{
		/*
		entVec.resize( entVec.size() +1 ) ;
		entVec.back().first = entId;
		entVec.back().second = teli;
		*/
	}

	StoredToken( ) : 
		tokId(INVALID_STORED_ID), 
		stringId(ay::UniqueCharPool::ID_NOTFOUND),
		numWords(0), length(0) 
	{}

	uint32_t getTrieId( ) const { return stringId; }

	/// initializes this token as a compounded word 
	void setCompounded( uint32_t tid, uint32_t cwId, uint16_t nw, uint16_t len  ) 
	{
		tokId = tid;
		stringId = cwId;
		numWords = nw;
	}

	// top level print method - single line abbreviated	
	void print( std::ostream& ) const;

	uint32_t getId() const { return tokId; }

	uint32_t getSingleTokStringId() const
		{ return( isSimpleTok() ? stringId : 0xffffffff ); }
	uint32_t getStringId() const { return stringId; }
};
inline std::ostream& operator <<( std::ostream& fp, const StoredToken& t )
{ return ( t.print(fp), fp ); }


/// information stored together with a token id within a "name" 
/// each Entity may have a few privileged token sequences which are 
/// treated as names. This is the way to take the order of tokens 
/// into account. 
/// at first everything will just have nameId 0 which means an unordered 
/// assortment of linked tokens
struct EntTokenOrderInfo {
	uint16_t nameId; // sequence id. 0 means an unordered collection of tokens 
	uint8_t  idx;   // index within sequence. for nameId this value is irrelevant
	// bitflags
	enum {
		ETOIBIT_EUID // tis token is a euid (unique id)
	};
	ay::bitflags<8> bits;   // index within sequence. for nameId this value is irrelevant

	inline bool isEuid() const { return bits[ETOIBIT_EUID] ; }
	inline void setBit_Euid( ) { bits.set(ETOIBIT_EUID); }

	void incrementName() { ++nameId; }
	void incrementIdx() { ++idx; }

	EntTokenOrderInfo() : nameId(0), idx(0) {}
}; 
typedef std::pair< StoredTokenId, EntTokenOrderInfo > StoredTokenSeqInfo_pair;
/// Stored Entity - thes objects live in DataIndex - they're only modified on load

/// no need to define == StoredEntityClass - it's a bcs class

struct StoredEntity {
	StoredEntityId entId; // entity id unique across all classes

	StoredEntityUniqId euid;
	
	int32_t relevance; // 0 - 2,000,000,000
	// typedef std::vector<StoredTokenSeqInfo_pair> STSI_vec; 
	// STSI_vec tokInfoVec; 

	StoredEntity() : 
		entId(INVALID_STORED_ID), 
		relevance(0) {}
	StoredEntity( StoredEntityId id, const StoredEntityUniqId& uniqId ) : 
		entId(id), 
		euid(uniqId),
		relevance(0) {}

	/*
	inline void addToken( StoredTokenId tokId, const EntTokenOrderInfo& stsi )
	{
		tokInfoVec.push_back( STSI_vec::value_type( tokId, stsi ) ) ;
	}
	inline bool hasTokenLinSrch( StoredTokenId tokId ) const
		{ 
			for( STSI_vec::const_iterator i = tokInfoVec.begin(); i!= tokInfoVec.end(); ++i ) {
				if( i->first == tokId )
					return true;
			}
			return false;
		}
	*/
	void setAll( StoredEntityId id, const StoredEntityUniqId& uniqId )
		{ entId= id; euid = uniqId; }
	
	const StoredEntityUniqId& getEuid() const { return euid; }
	const StoredEntityClass& getEclass() const { return euid.eclass; }

    uint32_t getClass() const { return euid.eclass.ec; }
    uint32_t getSubclass() const { return euid.eclass.subclass; }

	void print( std::ostream& ) const;
};
inline std::ostream& operator <<( std::ostream& fp, const StoredEntity& e )
{
	return ( e.print(fp), fp );
}


}
#endif // BARZER_STORAGE_TYPES_H
