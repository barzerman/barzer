#ifndef BARZER_DTAINDEX_H
#define BARZER_DTAINDEX_H

#include <barzer_storage_types.h>
#include <ay/ay_slogrovector.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util_char.h>

namespace barzer {

struct CompWordsTree;
struct SemanticalFSA;
struct StoredEntityPool;
struct StoredTokenPool;


class StoredEntityPool {
	ay::slogrovector<StoredEntity> storEnt;

	typedef std::map< StoredEntityUniqId, StoredEntityId > UniqIdToEntIdMap;
	UniqIdToEntIdMap euidMap;

	friend class DtaIndex;
	void clear()
		{ storEnt.vec.clear(); }
	StoredEntity& getEnt( StoredEntityId id ) { return storEnt.vec[ id ]; }
public:	
	/// first argument is set to true if new StoredEntity was created 
	/// otherwise it's set to false
	StoredEntity& addOneEntity( bool&, const StoredEntityUniqId& );

	const StoredEntity& getEnt( StoredEntityId id ) const { return storEnt.vec[ id ]; }
	bool isEntityIdValid( const StoredEntityId entId )  const
		{ return (entId < storEnt.vec.size() ); }

	void setPoolCapacity( size_t initCap, size_t capIncrement ) 
		{ storEnt.setCapacity(initCap,capIncrement); }
};
	
/// pool of all stored tokens 
class StoredTokenPool
{
private:
	friend class DtaIndex;
	ay::UniqueCharPool* strPool; // must be a global pool
	
	ay::slogrovector<StoredToken> storTok;

	/// single tokens mapped by the actual const char*
	/// references ponters stored in storTok
	ay::map_by_char_cp<StoredToken*>::Type singleTokMap;
	void clear() 
		{ 
			singleTokMap.clear();
			storTok.vec.clear();
		}
public:
	void setPoolCapacity( size_t initCap, size_t capIncrement ) 
		{ storTok.setCapacity(initCap,capIncrement); }

	StoredTokenPool( ay::UniqueCharPool* sPool ) : 
		strPool(sPool)
	{}
	const StoredToken* getTokByString( const char* s ) const
		{
			typeof( singleTokMap.begin() ) i = singleTokMap.find( s) ;
			return ( i == singleTokMap.end() ? 0 : i->second );
		}
	StoredToken* getTokByString( const char* s ) 
		{
			typeof( singleTokMap.begin() ) i = singleTokMap.find( s) ;
			return ( i == singleTokMap.end() ? 0 : i->second );
		}
	/// newAdded is set to true only if this called resulted in creation of a new token 
	/// set to false otherwise
	StoredToken& addSingleTok( bool& newAdded, const char* t);
	
	bool isTokIdValid(StoredTokenId id ) const
	{ return ( id< storTok.vec.size() ); }

	// boundaries are not checked. generally this id must be vetted 
	const StoredToken& getTokById( StoredTokenId id )  const
		{ return storTok.vec[ id ]; }
		  StoredToken& getTokById( StoredTokenId id ) 
		{ return storTok.vec[ id ]; }
};

/// 
/// DTAINDEX stores all tokens, entities as well as compounded words prefix trees as well as 
/// semantical FSAs. It is loaded once and then accessed read-only during parsing
/// some elements can be reloaded at runtime (details of that are TBD)
class DtaIndex {
	ay::UniqueCharPool* strPool; // must be a global pool
public: 
	StoredTokenPool tokPool;
	StoredEntityPool entPool;
private:
	StoredToken* getStoredToken( const char* s) 
		{ return tokPool.getTokByString( s ); }

	StoredToken& getStoredTokenById( StoredTokenId id ) 
		{ return tokPool.getTokById(id); }
public:
	const ay::UniqueCharPool* getStrPool() const { return strPool; }

	CompWordsTree* cwTree;
	SemanticalFSA* semFSA;

	/// given the token returns a matching StoredToken 
	/// only works for imple tokens 
	const StoredToken* getStoredToken( const char* s) const
		{ return tokPool.getTokByString( s ); }

	const StoredToken& getStoredTokenById( StoredTokenId id ) const
		{ return tokPool.getTokById(id); }

	// these two methods add token and entity honoring ordering info and link info (teli) 
	// as they get references to one another. Modifies both token and entity objects 
	// if unique is true only adds the token if it's either non-0 name id or 
	// it's unique. which may result in a load time hit
	// called from load

	// adds token to the token pool. 
	void addTokenToEntity(
		const char* tok,
		StoredEntity& ent, 
		const EntTokenOrderInfo& ord, 
		const TokenEntityLinkInfo& teli,
		bool unique=false );

	void addTokenToEntity(
		StoredToken& stok,
		StoredEntity& ent, 
		const EntTokenOrderInfo& ord, 
		const TokenEntityLinkInfo& teli,
		bool unique=false );

	void clear() ;
	~DtaIndex();
	DtaIndex( ay::UniqueCharPool* sPool ); // must be a global pool

	int loadEntities_XML( const char* fileName );

}; 

} // namespace barzer
#endif //  BARZER_DTAINDEX_H
