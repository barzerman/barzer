#ifndef AY_STRING_POOL_H 
#define AY_STRING_POOL_H 

/// alllocation pool functionality 
#include <vector>
#include <ay_util_char.h>

namespace ay {

typedef std::vector<char_p> char_p_vec;	

//// pools const char* strings. uniqueness is not enforced  
class CharPool {
	size_t chunkSz;
	/// chunks can be of different sizes 
	typedef std::vector< char_p_vec > ChunkVec;
	ChunkVec chunk;	
protected:
	char_p_vec& addNewChunk( ) ;
public:
	// len should include terminal 0
	const char* addStringToPool( const char* s, size_t len );
	// pushes new string into the pool 
	const char* addStringToPool( const char* s )
		{return( s ? addStringToPool(s,strlen(s)+1) : "" ); }
		
	enum { DEFAULT_CHUNK_SIZE = 64*1024 };
	CharPool( size_t cSz = DEFAULT_CHUNK_SIZE );
};

/// stores each string only once 
/// also issues "string ids" - 4-byte integers
class UniqueCharPool : public CharPool {
public:
	typedef uint32_t StrId;

	enum { 
		ID_NOTFOUND = 0xffffffff 
	};
private:
	char_cp_vec idVec; // idVec[StrId] is the char_cp
	map_by_char_cp<StrId>::Type idMap; // idMap[char_cp] returns id 
		// ID_NOTFOUND when string cant be found
public: 

	StrId getId( const char* s ) const
		{ 
			map_by_char_cp<StrId>::Type::const_iterator i= idMap.find(s);
			return ( i == idMap.end() ? ID_NOTFOUND : i->second );
		}
	const char* resolveId( StrId id ) const
		{ return ( (id< idVec.size()) ? idVec[id] : 0 ); }

	StrId internIt( const char* s ); 
	UniqueCharPool( size_t cSz = DEFAULT_CHUNK_SIZE ) : 
		CharPool(cSz ) { }

};

}
#endif // AY_STRING_POOL_H 
