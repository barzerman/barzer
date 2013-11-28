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
    typedef const typename tagindex<I>::IdSet IdSetPtr;
    std::vector<IdSetPtr> d_setVec;
public: 
    tagindex_checker( ) : d_idx(0) {}
    tagindex_checker( const tagindex<I>* i ) : d_idx(i) {}

    void setIdx(const tagindex<I>* i ) { 
        d_idx = i; 
        d_setVec.clear();
    }
    bool empty() const { return ( !d_idx || d_setVec.empty() ); }
    bool addTag( const char* t ) 
    {
        if( !d_idx ) return false;
        auto x = d_idx->find( t );
        if( x != d_idx->end() ) 
            d_setVec.push_back( &(x->second) );
        return true;
    }
    bool hasAnyTags( const I& id ) 
    {
        if( empty() ) return false;
        for( const auto& s : d_setVec ) {
            if( s->find(id) != s->end() )
                return true;
        }
        return false;
    }
    bool hasAllTags( const I& id ) 
    {
        if( empty() ) return false;
        for( const auto& s : d_setVec ) {
            if( s->find(id) == s->end() )
                return false;
        }
        return true;
    }
    /// callback cb will be invoked for every id linked to all of the Tags as many times as there are tags this id is linked to 
    /// call is cb( id, tag )
    template <typename CB>
    size_t visitAllIds( const CB& cb )
    {
        if( empty() ) return 0;
        #warning traverse sets one by one  
    }
};

} // namespace ay
