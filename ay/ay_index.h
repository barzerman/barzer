#pragma once 
#include <vector>
#include <map>
#include <algorithm>
namespace ay {

/// for every value theres a set of docIds
template <typename T>
class IdValIndex {
public:
    typedef std::pair< T, uint32_t > ValIdPair_t;

    struct compare_eq {
        bool operator()( const ValIdPair_t& l, const ValIdPair_t& r ) const
            { return (l.second == r.second && l.first == r.first) ; }
    };
    struct compare_less {
        bool operator()( const ValIdPair_t& l, const ValIdPair_t& r ) const
        { 
            return(  l.first < r.first ? true :
                ( r.first < l.first ? false : (l.second < r.second))
            );
        }
    };
    struct compare_less_byid {
        bool operator()( const ValIdPair_t& l, const ValIdPair_t& r ) const
        { 
            return(  l.second < r.second ? true :
                ( r.second < l.second ? false : (l.first < r.first))
            );
        }
    };

    // MUST BE SORTED in ascending order
    std::vector< ValIdPair_t > d_vec;     // sorted using compare_less
    std::vector< ValIdPair_t > d_vecById; // same pairs as d_vec except sorted by id (compare_less_byid)


    // val MUST BE SORTED in ascending order
    bool isOneOf( uint32_t docId , const std::vector<T>& val ) const
    {
        auto vecIter = d_vec.begin();
        for( auto v = val.begin(); v != val.end(); ++v ) {
            ValIdPair_t p({ *v, docId });
            vecIter = std::lower_bound( vecIter, d_vec.end(), p, compare_less() );
            if( vecIter == d_vec.end() )
                return false;
            if( compare_eq()( *vecIter, p ) )
                return true;
        }

        return false;
    }
    /// the range is inclusive
    bool isInRange( uint32_t docId, const T& l, const T& r ) const
    {
        ValIdPair_t p({ l, docId });
        auto i = std::lower_bound( d_vec.begin(), d_vec.end(), p, compare_less_byid() );
        return( (i == d_vec.end() || i->second != docId) ?  false : !(r< i->first) );
    }
    
    // iterates over all pairs in range
    // CB should return false if it wishes iteration to stop
    // it takes const ValIdPair_t
    template <typename CB>
    void iterateValue( const CB& cb, const T&l, const T& r )  const
    {
        ValIdPair_t p({ l, 0 });
        for( auto i = std::lower_bound( d_vec.begin(), d_vec.end(), p, compare_less_byid() ); i!= d_vec.end() && !(i->first <l) && !(r< i->first); ++i ) {
            if( !cb( *i ) ) 
                return;
        }
    }

    void append( uint32_t docId, const T& val ) 
        { 
            d_vec.push_back({ val, docId }); 
            d_vecById.push_back({ val, docId }); 
        }

    /// must be called once everything has been loaded 
    void sort() { 
        std::sort( d_vec.begin(), d_vec.end(), compare_less() ); 
        std::sort( d_vecById.begin(), d_vecById.end(), compare_less_byid() ); 
    }
    void clear() { 
        d_vec.clear(); 
        d_vecById.clear(); 
    }
};

template <typename T>
struct IdPropValIndex {
public:
    typedef IdValIndex<T> IdxType_t;
    typedef typename IdxType_t::ValIdPair_t ValIdPair_t;
private:
    std::map<std::string, IdxType_t> d_idxMap;
public:
    const IdValIndex<T>* getPropIdx( const std::string& propName ) const 
    {
        auto i = d_idxMap.find( propName );
        return( i == d_idxMap.end() ? 0: &(i->second) );
    }
    IdValIndex<T>& producePropIdx( const std::string& propName )
    {
        auto i = d_idxMap.find( propName );
        if( i == d_idxMap.end() ) 
            i = d_idxMap.insert( { propName, IdValIndex<T>()} ).first;
        return i->second;
    }

    void append( const std::string& propName, uint32_t docId, const T& val ) 
        { producePropIdx(propName).append( docId, val ); }
    void sort()
        { for( auto& i: d_idxMap ) i.second.sort(); }
    void clear() 
        { for( auto& i: d_idxMap ) i.second.clear(); }
};


} // namespace ay
