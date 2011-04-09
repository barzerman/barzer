#ifndef AY_POOL_WITH_ID_H
#define AY_POOL_WITH_ID_H

#include <vector>
#include <map>

/// pool of objects - vector+map pointing at each other 
/// map T --> uint32_t
/// vector is of const T* . offset in the vector is the uint32_t in the map
/// pointer points into the map
/// generally speaking this is good enough especially if we know the size of 
/// the set ahead of time. in the future a smarter pool can be implemented 
namespace ay {

template <typename T>
class InternerWithId {
public:
	typedef uint32_t IdType;
	enum {INVALID_ID = 0xffffffff, DEFAULT_VECSZ=32*1024 };
private:
	typedef T* T_p;
	typedef const T* T_cp;
	typedef std::vector<T_cp> TpVec;
	typedef std::map<T,uint32_t> T2IdMap;
	
	TpVec theVec;
	T2IdMap theMap;
	
	IdType pushNewObj( T_p tp ) 
		{ 
			IdType idx = theVec.size();
			theVec.push_back( tp ) ;
			return idx;
		}
public:
	InternerWithId( size_t sz = DEFAULT_VECSZ )
		{ theVec.reserve( sz ); }
	
	void reserve( size_t sz ) 
		{ theVec.reserve(sz); }
	
	uint32_t getVecSize( ) const 
		{ return theVec.size() ; }

	T_cp getObjById( IdType id ) const 
		{ return( id< theVec.size() ? theVec[id] : 0 ); }
	
	inline IdType getIdByObj( const T& t ) const
	{
		typename T2IdMap::const_iterator i = theMap.find( t );
		return ( i == theMap.end() ? INVALID_ID : i->second );
	}
	inline IdType produceIdByObj( const T& t )
	{
		typename T2IdMap::iterator i = theMap.find( t );
		if( i != theMap.end() ) 
			return i->second;
		i = theMap.insert( T2IdMap::value_type( t, INVALID_ID ) ).first;
		return( (i->second = pushNewObj( &(i->first) )) );
	}
};

/// simple PoolWithId does not ensure uniqueness it simply stores 
/// objects in consequtive chunkconsts and issue ids . 
/// access by id is O(1)
/// T must have a default constructor
template <typename T>
class PoolWithId {
public:
	enum { DEFAULT_CHUNK_CAPACITY = 1024 * 16 };
private:
	typedef T* Chunk;
	size_t chunkCapacity;

	size_t curChunkSz;
	Chunk  curChunk;

	typedef std::vector< Chunk > ChunkVec;
	ChunkVec cVec;

	void addNewChunk( ) 
	{
		cVec.resize( cVc.size() +1 );
		cVec.back() = new T[ chunkCapacity ];
		curChunk = cVec.back();
		curChunkSz = 0;
	}
		
public:
	T* addObj( uint32_t& id ) 
	{
		if( curChunkSz>= chunkCapacity ) 
			addNewChunk();
		id = (cVec.size() *chunkCapacity) +  curChunkSz;
		T* t =  curChunk + curChunkSz;
		++curChunkSz;
		return t;
	}

	const T* getObjById( uint32_t id )  const
	{
		size_t chunkId = ( id / chunkCapacity );
		if( chunkId < cVec.size() ) {
			size_t offset = ( id% chunkCapacity );
			return( offset < curChunkSz? (curChunk+offset) : 0 );
		} else
			return 0;
	}
	T* getObjById( uint32_t id )  
		{ return ( (T*) ( ((const PoolWithId*)this )->getObjById(id) ) ); }

};


}

#endif // AY_POOL_WITH_ID_H
