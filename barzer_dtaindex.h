#ifndef BARZER_DTAINDEX_H
#define BARZER_DTAINDEX_H

#include <ay/ay_headers.h>
#include <barzer_storage_types.h>
#include <ay/ay_vector.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util_char.h>
#include <boost/unordered_map.hpp>

namespace barzer {

struct CompWordsTree;
struct SemanticalFSA;
class StoredEntityPool;
class StoredTokenPool;


class StoredEntityPool {
	ay::slogrovector<StoredEntity> storEnt;

	typedef std::map< StoredEntityUniqId, StoredEntityId > UniqIdToEntIdMap;
	typedef std::map< StoredEntityClass, uint32_t > EclassStatsMap;

	UniqIdToEntIdMap euidMap;

	EclassStatsMap eclassMap;

	friend class DtaIndex;
	void clear()
		{ 
			eclassMap.clear();
			euidMap.clear();
			storEnt.vec.clear(); 
		}
	StoredEntity& getEnt( StoredEntityId id ) { return storEnt.vec[ id ]; }
	
	void addOneToEclass( const StoredEntityClass& eclass) {
		EclassStatsMap::iterator i = eclassMap.find( eclass );
		if( i == eclassMap.end() ) 
			i = eclassMap.insert( EclassStatsMap::value_type( eclass, 0 ) ).first;
		
	}
public:	
	uint32_t getEclassEntCount( const StoredEntityClass& eclass ) const 
	{
		EclassStatsMap::const_iterator i = eclassMap.find( eclass );
		return( i == eclassMap.end() ? 0: i->second );
	}
	inline const StoredEntityId getEntIdByEuid( const StoredEntityUniqId& euid ) const
	{
		UniqIdToEntIdMap::const_iterator i = euidMap.find( euid );

		return( i== euidMap.end() ?  INVALID_STORED_ID : i->second ) ;
	}
	
	inline const StoredEntity* getEntByEuid( const StoredEntityUniqId& euid ) const
	{
		UniqIdToEntIdMap::const_iterator i = euidMap.find( euid );

		if( i== euidMap.end() ) 
			return 0;
		StoredEntityId entId = i->second;
		return ( &(storEnt.vec[entId]) );
	}
	/// first argument is set to true if new StoredEntity was created 
	/// otherwise it's set to false
	StoredEntity& addOneEntity( bool&, const StoredEntityUniqId& );

	const StoredEntity& getEnt( StoredEntityId id ) const { return storEnt.vec[ id ]; }
	bool isEntityIdValid( const StoredEntityId entId )  const
		{ return (entId < storEnt.vec.size() ); }

	void setPoolCapacity( size_t initCap, size_t capIncrement ) 
		{ storEnt.setCapacity(initCap,capIncrement); }
	void print(std::ostream& fp ) const
	{
		fp << "euid: " << euidMap.size() ; 
	}
	/// checks boundaries returns 0 if id is invalid
	inline const StoredEntity* getEntByIdSafe( StoredEntityId id )  const
		{ return( isEntityIdValid(id) ? &(getEnt(id)):(const StoredEntity*)0 );}
	size_t getNumberOfEntities() const
		{ return euidMap.size(); }
};

inline std::ostream& operator <<( std::ostream& fp, const StoredEntityPool& x )
{
	x.print(fp);
	return fp;
}

/// pool of all stored tokens 
class StoredTokenPool
{
private:
	friend class DtaIndex;
	ay::UniqueCharPool* strPool; // must be a global pool
	
	ay::slogrovector<StoredToken> storTok;

	/// single tokens mapped by the actual const char*
	/// references ponters stored in storTok
	typedef std::map<const char*,StoredTokenId,ay::char_cp_compare_nocase_less> SingleTokMap;
	SingleTokMap singleTokMap;

	// cwid to offset in storTok
	typedef boost::unordered_map<uint32_t, uint32_t> CwidToTokenIdMap;
	CwidToTokenIdMap d_cwidMap;
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
	StoredTokenId getTokIdByString( const char* t ) const
	{
		SingleTokMap::const_iterator i = singleTokMap.find( t) ;
		return (  i == singleTokMap.end() ? INVALID_STORED_ID : i->second );
	}
	const StoredToken* getTokByString( const char* s ) const
		{
			SingleTokMap::const_iterator i = singleTokMap.find( s) ;
			//typeof( singleTokMap.begin() ) i = singleTokMap.find( s) ;
			return ( i == singleTokMap.end() ? 0 : &(storTok.vec[i->second]) );
		}
	StoredToken* getTokByString( const char* s ) 
		{
			return (StoredToken*)(((const StoredTokenPool*)this)->getTokByString(s));
		}
	/// newAdded is set to true only if this called resulted in creation of a new token 
	/// set to false otherwise
	StoredToken& addSingleTok( bool& newAdded, const char* t);
	/// adds new compounded word
	StoredToken& addCompoundedTok( bool& newAdded, uint32_t cwId, uint16_t numW, uint16_t len );
	
	bool isTokIdValid(StoredTokenId id ) const
	{ return ( id< storTok.vec.size() ); }

	// boundaries are not checked. generally this id must be vetted 
	const StoredToken& getTokById( StoredTokenId id )  const
		{ return storTok.vec[ id ]; }
  		  StoredToken& getTokById( StoredTokenId id ) 
		{ return storTok.vec[ id ]; }
	void print(std::ostream& fp) const 
		{ fp <<  "toks:" <<  singleTokMap.size() ; }

	const StoredToken* getTokByCwid( uint32_t cwid ) const 
	{
		CwidToTokenIdMap::const_iterator i = d_cwidMap.find( cwid );
		return( i == d_cwidMap.end() ? 0: &(getTokById(i->second)) );
	}
	StoredToken* getTokByCwid( uint32_t cwid ) 
		{ return const_cast<StoredToken*>(  ((const StoredTokenPool*)this)->getTokByCwid(cwid) ); }

	/// checks boundaries returns 0 if id is invalid
	inline const StoredToken* getTokByIdSafe( StoredTokenId id )  const
		{ return( isTokIdValid(id) ? &(getTokById(id)):0 );}
	size_t getNumberOfTokens() const
		{ return singleTokMap.size(); }
	
};

inline std::ostream& operator <<( std::ostream& fp, const StoredTokenPool& x )
{
	x.print( fp );
	return fp;
}
class GlobalPools;
/// 
/// DTAINDEX stores all tokens, entities as well as compounded words prefix trees as well as 
/// semantical FSAs. It is loaded once and then accessed read-only during parsing
/// some elements can be reloaded at runtime (details of that are TBD)
class DtaIndex {
    GlobalPools& d_gp;
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

	size_t getNumberOfTokens() const { return tokPool.getNumberOfTokens(); }
	size_t getNumberOfEntities() const { return entPool.getNumberOfEntities(); }
	/// given the token returns a matching StoredToken 
	/// only works for imple tokens 
	const StoredToken* getStoredToken( const char* s) const
		{ return tokPool.getTokByString( s ); }

	const StoredToken& getStoredTokenById( StoredTokenId id ) const
		{ return tokPool.getTokById(id); }

	const StoredToken* getStoredTokenPtrById( StoredTokenId id ) const
		{ return tokPool.getTokByIdSafe(id); }


	StoredTokenId getTokIdByString( const char* t ) const
        { return tokPool.getTokIdByString(t); }
	/// reads token, eclass, subclass from stream and constructs Euid if and only iff
	/// all three could be read and the token could be resolved
	/// Returns false if fails to read all three components or resolve the token 
	bool buildEuidFromStream( StoredEntityUniqId& euid, std::istream& ) const;

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

	/*
	void addTokenToEntity(
		StoredToken& stok,
		StoredEntity& ent, 
		const EntTokenOrderInfo& ord, 
		const TokenEntityLinkInfo& teli,
		bool unique=false );
		*/

	void clear() ;
	~DtaIndex();
	DtaIndex( GlobalPools&, ay::UniqueCharPool* ); // must be a global pool

	int loadEntities_XML( const char* fileName );

	inline const StoredEntity* getEntByEuid( const StoredEntityUniqId& euid ) const
		{ return entPool.getEntByEuid(euid); }
	inline const StoredToken* getTokByString( const char* s ) const
		{ return tokPool.getTokByString(s); }
	void print( std::ostream&  fp ) const;

	inline const char* resolveStringById( uint32_t id  ) const  
	{ return strPool->resolveId( id ); }
	inline const char* resolveStoredTokenStr( const StoredToken& tok ) const 
	{
		if( tok.isSimpleTok() ) {
			return strPool->resolveId(tok.stringId);
		} else {
			
			return "(compound)";
		}
	}
	/// never returns 0
	inline const char* resolveStoredTokenStr( StoredTokenId id ) const 
	{
		const StoredToken* t = tokPool.getTokByIdSafe( id );
		return ( t ? resolveStoredTokenStr(*t) : "" );
	}
	void printStoredToken( std::ostream& fp, const StoredTokenId ) const;
	void printEuid( std::ostream& fp, const StoredEntityUniqId& euid ) const
	{ fp << euid << '|' << resolveStoredTokenStr(euid.tokId ); }
	
	StoredToken& addToken( bool& wasNew, const char* t ) {
		// bool wasNew = false;
		return tokPool.addSingleTok( wasNew, t );
	}
	StoredToken& addToken( const char* t ) {
		bool wasNew = false;
		return tokPool.addSingleTok( wasNew, t );
	}
	enum {
		COMPTOK_DEFAULT_NUMW = 2,
		COMPTOK_DEFAULT_LEN  = 10
	};
	StoredToken& addCompoundedToken( uint32_t cwId ) {
		bool wasNew = false;
		return tokPool.addCompoundedTok( wasNew, cwId, COMPTOK_DEFAULT_NUMW, COMPTOK_DEFAULT_LEN );
	}
	StoredToken& addCompoundedToken( uint32_t cwId, uint16_t numW, uint16_t len ) {
		bool wasNew = false;
		return tokPool.addCompoundedTok( wasNew, cwId, numW, len );
	}
	/// add generic entity does not link storedtoken and the newly minted entity
	StoredEntity& addGenericEntity( uint16_t cl, uint16_t scl )
	{
		const StoredEntityUniqId euid( 0xffffffff, cl, scl );
		bool isNew = false;
		return entPool.addOneEntity( isNew, euid );
	}
	StoredEntity& addGenericEntity( uint32_t t, uint16_t cl, uint16_t scl )
	{
		const StoredEntityUniqId euid( t, cl, scl );

		bool isNew = false;
		return entPool.addOneEntity( isNew, euid );
	}
    
	
	const StoredEntity* getEntById( uint32_t id ) const
	{
		return entPool.getEntByIdSafe(id);
	}
	uint32_t getEclassEntCount( const StoredEntityClass& eclass ) const 
		{ return entPool.getEclassEntCount( eclass ); }
}; 
/// EntPropCompatibility is an index used to determine whether a particular entity can serve as a property for 
/// another entity
///
/// this is in essence a rule set
/// rules are in 1 on 4 classes:
///   1. Entity Eclass <---> Prop Eclass (example: (1,1) <----> (10,10) )  (1,1,0xffffffff), (10,10,0xfffffff) 
///  
///   2. Entity Eclass <----> Prop Euid   (example (1,1)     <----> (10,10,fgh)

///   3. Euid          <----> Prop Eclass (example (1,1,abc) <-----> (10,10) 
///   4. Euid          <----> Prop Euid   (example (1,1,abc) <----> (10,10,fgh) )

class EntPropCompatibility {
	typedef std::pair< StoredEntityUniqId, StoredEntityUniqId > EntPropPair;
	typedef std::set< EntPropPair > EntPropPairSet;

	EntPropPairSet d_entPropPairSet;
	
public:
	bool entPropPairs( const StoredEntityUniqId& l, const StoredEntityUniqId& r ) const 
	{
		{ // rule type 1 
		if( d_entPropPairSet.find( EntPropPair( StoredEntityUniqId(l.eclass), StoredEntityUniqId(r.eclass)) ) != d_entPropPairSet.end() ) 
			return true;
		}	
		{ // rule type 2
		if( d_entPropPairSet.find( EntPropPair( StoredEntityUniqId(l.eclass), r ) ) != d_entPropPairSet.end() ) 
			return true;
		}	
		{ // rule type 3
		if( d_entPropPairSet.find( EntPropPair( l, StoredEntityUniqId(r.eclass) ) ) != d_entPropPairSet.end() ) 
			return true;
		}	
		{ // rule type 4
		if( d_entPropPairSet.find( EntPropPair(l,r) ) != d_entPropPairSet.end() ) 
			return true;
		}	
		return false;
	}

	void addRule( const StoredEntityUniqId& l, const StoredEntityClass& ec ) 
		{ d_entPropPairSet.insert( EntPropPair(l,StoredEntityUniqId(ec)) ) ; }
	void addRule( const StoredEntityUniqId& e, const StoredEntityUniqId& p ) 
		{ d_entPropPairSet.insert( EntPropPair(e,p) ); }
	void addRule( const StoredEntityClass& e, const StoredEntityUniqId& p ) 
		{ d_entPropPairSet.insert( EntPropPair(StoredEntityUniqId(e),p) ); }
	void addRule( const StoredEntityClass& e, const StoredEntityClass& p ) 
		{ d_entPropPairSet.insert( EntPropPair(StoredEntityUniqId(e),StoredEntityUniqId(p)) ); }
};

} // namespace barzer
#endif //  BARZER_DTAINDEX_H
