#include <ay_string_pool.h>

namespace ay {

CharPool::CharPool( size_t cSz ) : chunkSz(cSz)
{
	addNewChunk();
}

char_p_vec& CharPool::addNewChunk( )
{
	chunk.resize( chunk.size() + 1 );
	chunk.back().reserve( chunkSz );
}

const char* CharPool::addStringToPool( const char* s, size_t len )
{
	if( len > chunkSz ) 
		chunkSz = len;
	if( chunk.back().capacity() < chunk.back().size() + len ) 
		addNewChunk();
	
	char* loc = chunk.back().back();
	chunk.back().resize( chunk.back().size() + len ) ;
	memcpy( loc+1, s, len );
	return loc;
}
///// UniqueCharPool
UniqueCharPool::StrId UniqueCharPool::internIt( const char* s ) 
{
	StrId id = getId(s);
	if( id>=0 ) 
		return id;
	
	const char * newS = addStringToPool( s );
	id = idVec.size();
	idVec.push_back( newS );
	idMap[newS]= id;
	
	return id;
}

}
