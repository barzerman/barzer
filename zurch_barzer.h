#pragma once 

#include <barzer_entity.h>
#include <map>

namespace barzer{ 
struct StoredEntityClass;
}

/// links zurch and barzer
namespace zurch {

class DocIndexLoaderNamedDocs;

using barzer::BarzerEntity;

class BarzerEntityDocLinkIndex {
public:
    typedef std::multimap< BarzerEntity, uint32_t > Ent2DocIdMap;
    typedef std::multimap< uint32_t, BarzerEntity > Docid2EntMap;
private:
    Ent2DocIdMap d_ent2DocId;
    Docid2EntMap d_docId2Ent;
    
public:
    DocIndexLoaderNamedDocs& d_zurchLoader;
    BarzerEntityDocLinkIndex( DocIndexLoaderNamedDocs& loader ): d_zurchLoader(loader) {}

    std::string d_fileName;
    /// when arg = 0 uses d_fileName
    /// otherwise sets it
    /// by default the file is assumed to be pipe delimited  
    int loadFromFile( const std::string& fname );
    void addLink( const BarzerEntity& ent, const std::string& s );

    /// iterates all docs for given entity 
    /// CB must have operator()( uint32_t )
    template <typename CB>
    void iterate( const CB& cb, const BarzerEntity& ent )  const
        { for( auto i = d_ent2DocId.lower_bound( ent ); i!= d_ent2DocId.end() && i->first == ent; ++i ) cb( i->second ); }
    /// iterates all entities for given doc
    template <typename CB>
    void iterate( const CB& cb, uint32_t docId )  const
        { for( auto i = d_docId2Ent.lower_bound( docId ); i!= d_docId2Ent.end() && i->first == docId; ++i ) cb( i->second ); }
    
    void getEntVec( std::vector< BarzerEntity >& vec, uint32_t docId ) 
        { iterate( [&vec]( const BarzerEntity& ent ) { vec.push_back( ent ); }, docId); }
    void getEntSet( std::set< BarzerEntity >& x, uint32_t docId ) 
        { iterate( [&x]( const BarzerEntity& ent ) { x.insert( ent ); }, docId); }

    void getDocVec( std::vector< uint32_t >& vec, const BarzerEntity& ent )
        { iterate( [&vec]( uint32_t docId ) { vec.push_back( docId ); }, ent); }
    void getDocSet( std::set< uint32_t >& x, const BarzerEntity& ent )
        { iterate( [&x]( uint32_t docId ) { x.insert( docId ); }, ent); }
};

} // namespace zurch 
