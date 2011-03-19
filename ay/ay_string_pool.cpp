#include <ay_string_pool.h>

namespace ay {

CharPool::CharPool( size_t cSz ) : chunkSz(cSz)
{
	addNewChunk();
}

char_vec& CharPool::addNewChunk( )
{
	chunk.resize( chunk.size() + 1 );
	chunk.back().reserve( chunkSz );
}

const char* CharPool::addStringToPool( const char* s, size_t len )
{
	if( len > chunkSz ) 
		chunkSz = len;
	char_vec & backVec = chunk.back();
	if( backVec.capacity() < backVec.size() + len ) 
		addNewChunk();
	
	char* loc = &(backVec.back());
	backVec.resize( chunk.back().size() + len ) ;
	memcpy( loc+1, s, len );
	return loc;
}
///// UniqueCharPool
UniqueCharPool::StrId UniqueCharPool::internIt( const char* s ) 
{
	StrId id = getId(s);
	if( id!=ID_NOTFOUND ) 
		return id;
	
	const char * newS = addStringToPool( s );
	id = idVec.size();
	idVec.push_back( newS );
	idMap[newS]= id;
	
	return id;
}

}
