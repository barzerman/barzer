#include <ay_string_pool.h>
#include <iostream>

namespace ay {

void CharPool::clear() 
{
	for( ChunkVec::iterator i = chunk.begin(); i!= chunk.end(); ++i ) {
		free( *i );
		*i=0;
	}
	chunk.clear();
}

CharPool::~CharPool( )
{
	clear();
}

CharPool::CharPool( size_t cSz ) : 
	chunkCapacity(cSz),
	chunkSz(0)
{
	addNewChunk();
}

char* CharPool::addNewChunk( )
{
	chunk.resize( chunk.size() + 1 );

	chunk.back() = (char*)malloc( chunkCapacity );
	memset( chunk.back(), 0, chunkCapacity );
	chunkSz = 0;
	return chunk.back();
}

const char* CharPool::addStringToPool( const char* s, size_t len )
{
	if( len > chunkCapacity ) 
		chunkCapacity = len;
	if( chunkCapacity < chunkSz + len ) {
		addNewChunk();
	}
	char* backVec = chunk.back();
	
	size_t loc = chunkSz;
	chunkSz += len; 
	
	char* locStr = backVec+loc;

	memcpy( locStr, s, len );
	return locStr;
}

}
