#ifndef BARZER_DTAINDEX_H
#define BARZER_DTAINDEX_H

#include <barzer_storage_types.h>

namespace barzer {

struct CompWordsTree;
struct SemanticalFSA;
/// this object is a singleton. it stores all tokens, entities 
/// as well as compounded words prefix trees as well as emantical state 
/// machines
class DtaIndex {
	
public: 
	CompWordsTree* cwTree;
	SemanticalFSA* semFSA;

	/// given the token returns a matching StoredToken 
	/// only works for imple tokens 
	const StoredToken* getStoredToken( const char* ) const;
	const StoredToken* getStoredTokenById( StoredTokenId ) const;
}; 

} // namespace barzer
#endif //  BARZER_DTAINDEX_H
