#ifndef BARZER_DTAINDEX_H
#define BARZER_DTAINDEX_H

#include <barzer_storage_types.h>
#include <ay/ay_slogrovector.h>

namespace barzer {

struct CompWordsTree;
struct SemanticalFSA;
struct StoredEntityPool;
struct StoredTokenPool;


class StoredEntityPool {
	ay::slogrovector<StoredEntity> savEnt;

public:	
	std::map<StoredTokenId,StoredEntityId> uniqIdTokMap1; 
	std::map<StoredTokenId,StoredEntityId> uniqIdTokMap2;

	StoredEntity& addOneEntity() 
	{ 	
		StoredEntity& e = savEnt.extend(); 
		e.entId = savEnt.vec.size();
		return e;
	}
	void setPoolCapacity( size_t initCap, size_t capIncrement ) 
		{ savEnt.setCapacity(initCap,capIncrement); }
};
	
struct StoredTokenPool
{
};
/// 
/// DTAINDEX stores all tokens, entities as well as compounded words prefix trees as well as 
/// semantical FSAs. It is loaded once and then accessed read-only during parsing
/// some elements can be reloaded at runtime (details of that are TBD)
class DtaIndex {
public: 
	CompWordsTree* cwTree;
	SemanticalFSA* semFSA;

	/// given the token returns a matching StoredToken 
	/// only works for imple tokens 
	const StoredToken* getStoredToken( const char* ) const;
	const StoredToken* getStoredTokenById( StoredTokenId ) const;
	~DtaIndex();
}; 

} // namespace barzer
#endif //  BARZER_DTAINDEX_H
