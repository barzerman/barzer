#ifndef AY_STRING_POOL_H 
#define AY_STRING_POOL_H 

/// alllocation pool functionality 
#include <ay_headers.h>
#include <vector>
#include <ay_util_char.h>
#include <stdint.h>
#include <boost/unordered_map.hpp>

namespace ay {


//// pools const char* strings. uniqueness is not enforced  
class CharPool {
	size_t chunkCapacity; // current chunk capacity
	size_t chunkSz;       // current chunk size

	/// chunks can be of different sizes 
	typedef std::vector< char* > ChunkVec;
	ChunkVec chunk;	
protected:
	char* addNewChunk( ) ;
public:
	// len should include terminal 0
	const char* addStringToPool( const char* s, size_t len );
	// pushes new string into the pool 
	const char* addStringToPool( const char* s )
		{return( s ? addStringToPool(s,strlen(s)+1) : "" ); }
		
	enum { DEFAULT_CHUNK_SIZE = 64*1024 };
	CharPool( size_t cSz = DEFAULT_CHUNK_SIZE );
	void clear();
	~CharPool();
};

/// default string serializer (escapes newlines)
struct StringSerializer {
    enum : size_t { DEFAULT_BUF_SZ = 16*1024 };
    std::vector<char> buf;
    char delim;

    StringSerializer( size_t sz = DEFAULT_BUF_SZ ) : buf(sz), delim('\n') {}
    bool operator()( std::ostream& fp, const char* s ) const
    {
        size_t sz = strlen(s);
        (fp <<  sz << " ").write( s, sz ) << '\n';
        return true;
    }
    bool operator()( std::ostream& fp, size_t s ) const
    {
        fp << s << '\n';
        return true;
    }
    bool operator()( std::string& s, std::istream& fp ) 
    {
        size_t sz = 0;
        fp >> sz;
        if( sz > buf.size() ) 
            sz = buf.size();
        if( !fp.read( &(buf[0]), sz+1 ) )  // +1 to account for the trailing newline
            return false;
        s.assign( &(buf[0]), sz );
        return true;
    }
    bool operator()( size_t& s, std::istream& fp ) 
    {
        fp.getline( &(buf[0]), buf.size()-1, delim );
        buf.back()= (char)(0);
        s = atoi( &(buf[0])) ;
        return( s> 0 ) ;
    }
};

/// stores each string only once 
/// also issues "string ids" - 4-byte integers
class UniqueCharPool : public CharPool {
public:
	typedef uint32_t StrId;

	enum { 
		ID_NOTFOUND = 0xffffffff 
	};
	// typedef std::map< char_cp, StrId, ay::char_cp_compare_less > CharIdMap;
	typedef boost::unordered_map< char_cp, StrId, char_cp_hash, ay::char_cp_compare_eq > CharIdMap;
private:
	char_cp_vec idVec; // idVec[StrId] is the char_cp
	CharIdMap idMap;
	// map_by_char_cp<StrId>::Type idMap; // idMap[char_cp] returns id 
		// ID_NOTFOUND when string cant be found
public: 
    template <typename SRLZR>
    int serialize( SRLZR& srlzr, std::ostream& fp )  const
    {
        srlzr( fp, idVec.size() );
        for( auto i = idVec.begin(); i!= idVec.end(); ++i ) {
            srlzr( fp, *i );
        }
        return 0;
    }
    template <typename SRLZR>
    int deserialize( SRLZR& srlzr, std::istream& fp )  
    {
        size_t sz = 0;
        if( !srlzr( sz, fp ) || !sz ) 
            return -1;

        std::string s;
        for( size_t i=0; i< sz && srlzr(s,fp); ++i )
            internIt(s.c_str());
        return 0;
    }
    int deserialize( std::istream& fp )
    {
        StringSerializer srlzr;
        return deserialize( srlzr, fp );
    }
    int serialize( std::ostream& fp ) const 
    {
        StringSerializer srlzr;
        return serialize( srlzr, fp );
    }
    const CharIdMap& getCharIdMap() const { return idMap; }
	void clear() 
	{
		idMap.clear();
		idVec.clear();
		CharPool::clear();
	}
	StrId getId( const char* s ) const
		{ 
			CharIdMap::const_iterator i= idMap.find(s);
			return ( i == idMap.end() ? ID_NOTFOUND : i->second );
		}
	const char* resolveId( uint32_t id ) const
		{ return ( (id< idVec.size()) ? idVec[id] : 0 ); }
	
	// this is guaranteed to never return 0
	inline const char* printableStr(uint32_t id) const
		{ 
			const char* s = resolveId(id);
			return( s? s: ""); 
		}
				
	inline StrId internIt( const char* s )
    {
        
	    StrId id = getId(s);
	    if( id!=ID_NOTFOUND ) {
		    return id;
	    }	
	    const char * newS = addStringToPool( s );
	    id = idVec.size();
	    idVec.push_back( newS );
	    idMap.insert( CharIdMap::value_type(newS, id) );
	
	    return id;
    }
    std::string d_junkBuf;

	inline StrId internIt( const char* s, size_t s_len )
    {
        if( !s[s_len] ) 
            return internIt(s);
        else {
            d_junkBuf.assign(s,s_len);
	        StrId id = getId(d_junkBuf.c_str());
	        if( id!=ID_NOTFOUND ) {
		        return id;
	        }	
	        const char * newS = addStringToPool( s, s_len );
	        id = idVec.size();
	        idVec.push_back( newS );
	        idMap.insert( CharIdMap::value_type(newS, id) );
        	
	        return id;
        }
    }
	UniqueCharPool( size_t cSz = DEFAULT_CHUNK_SIZE ) : 
		CharPool(cSz ) { }

    size_t getMaxId() const { return idVec.size(); }
    

};

}
#endif // AY_STRING_POOL_H 
