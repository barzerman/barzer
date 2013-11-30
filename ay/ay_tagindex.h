#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include <set>
#include <vector>

/// lookup table for a number of tagged ids
/// id is an integer type, tag is an ordered type
namespace ay {

template <typename I> class tagindex_checker ;

template <typename I>
class tagindex {
    typedef std::set<I> IdSet;
    typedef std::map<std::string,IdSet> TagMap;
    TagMap d_theMap;
    struct SetIter_comp_less { bool operator()( typename TagMap::const_iterator l, typename TagMap::const_iterator r ) const { return ( l->first < r->first ); } };
    typedef std::set<typename TagMap::const_iterator,SetIter_comp_less> TagMapIterVec;
    std::map< I, TagMapIterVec> d_id2TagIter;
public:
    typedef I id_type;
    const IdSet* getTagIdSetPtr( const char* t ) const
    {
        auto x = d_theMap.find(t);
        return( x == d_theMap.end() ? 0 : &(x->second) );
    }

    bool hasTag( const I& id, const char* tag )  const
    {
        auto i = d_theMap.find( tag );
        return( i == d_theMap.end() ? false : i->second.find(id) != i->second.end() );
    }

    template <typename CB>
    size_t visitAllIdsOfTag( const CB& cb, const char* tag ) const
    {
        auto x = d_theMap.find( tag );
        if( x == d_theMap.end())
            return 0;
        size_t count = 0;
        for( const auto& i : x->second ) {
            cb( i );
            ++count;
        }
        return count;
    }
    template <typename CB>
    size_t visitAllTagsOfId( const CB& cb, const I& id ) const
    {
        auto  i = d_id2TagIter.find( id );
        if( i == d_id2TagIter.end() ) 
            return 0;
        size_t count = 0;
        for( const auto& x : i->second ) {
            cb( x->first );
            ++count;
        }
        return count;
    }

    void addTag( const I& id, const char* tag ) 
    {
        auto x = d_theMap.find( tag );

        if( x == d_theMap.end() ) 
            x = d_theMap.insert( std::make_pair( std::string(tag), IdSet()) ).first;
        
        x->second.insert(id);

        auto  y = d_id2TagIter.find( id );
        if( y == d_id2TagIter.end() ) 
            y = d_id2TagIter.insert( std::make_pair( id, TagMapIterVec() ) ).first;
        
        y->second.insert( x );
    }
    friend class tagindex_checker<I>;
    void clear() { 
        d_id2TagIter.cleaar();
        d_theMap.clear(); 
    }
};
/// this class memoizes the tag and stores the ID set corresponding to the tag 
template <typename I>
class tagindex_checker {
    const tagindex<I>* d_idx;
    
    tagindex_checker( const tagindex_checker& ) {}
    typedef const typename tagindex<I>::IdSet* IdSetPtr;
    std::vector<IdSetPtr> d_setVec;
public: 
    tagindex_checker( ) : d_idx(0) {}
    tagindex_checker( const tagindex<I>* i ) : d_idx(i) {}


    void setIdx(const tagindex<I>* i ) { 
        d_idx = i; 
        d_setVec.clear();
    }
    bool empty() const { return ( !d_idx || d_setVec.empty() ); }
    const tagindex<I>& getIndex() { return *d_idx; }
    bool addTag( const char* t ) 
    {
        if( !d_idx ) return false;
        auto x = d_idx->getTagIdSetPtr( t );
        if( x )
            d_setVec.push_back( x );
        return true;
    }
    bool addTag( const std::string& t ) { return addTag( t.c_str()) ; }

    bool hasAnyTags( const I& id ) 
    {
        if( empty() ) return false;
        for( const auto& s : d_setVec ) {
            if( s->find(id) != s->end() )
                return true;
        }
        return false;
    }
    bool hasAllTags( const I& id ) const
    {
        if( empty() ) return false;
        for( const auto& s : d_setVec ) {
            if( s->find(id) == s->end() )
                return false;
        }
        return true;
    }
    /// callback cb will be invoked for every id linked to all of the Tags
    /// call is cb( id, tag )
    template <typename CB>
    size_t visitAllIds( const CB& cb ) const
    {
        if( empty() ) return 0;
        auto shortVecI = d_setVec.begin();
        /// looking up the shortest set 
        for( auto i = shortVecI+1; i!= d_setVec.end(); ++i ) 
            if( (*i)->size() > (*shortVecI)->size() )
                shortVecI = i;
        size_t count = 0;
        for( auto i = (*shortVecI)->begin(); i != (*shortVecI)->end(); ++i ) {
            bool goodForCallback = true;
            for( auto otherVec = d_setVec.begin(); otherVec!= d_setVec.end(); ++otherVec ) {
                if( otherVec != shortVecI && (*otherVec)->find( *i ) == (*otherVec)->end() ) {
                    goodForCallback= false;
                    break;
                }
            }
            if( goodForCallback ) {
                cb( *i );
                ++count;
            }
        }
        return count;
    }
    /// visits all applicable id,tag pairs    
    template <typename CB>
    size_t visitAllIdsAndTags( const CB& cb ) const
    {
        return visitAllIds( [&]( const I& id ) {
            d_idx->visitAllTagsOfId( []( const std::string& tag ) { cb( id, tag );}, id );
        });
    }
};

} // namespace ay
