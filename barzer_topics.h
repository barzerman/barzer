#ifndef BARZER_TOPICS_H
#define BARZER_TOPICS_H

// #include <boost/unordered_map.hpp> 
#include <map>
#include <barzer_entity.h>

namespace barzer {
class BELTrie;
/// f for Trie T the TrieTopics says something like positive: ( t1 || t2 ) negative: (t3 || t4 ) 
/// it would mean - Apply trie T is either t1 or t2 are found in the barz and neither t3 nor t4 are
class BarzTopics {
public:
    typedef std::map< BarzerEntity, int > TopicMap;
    typedef std::pair<BarzerEntity, int> EntWeight;
    
    typedef std::set<uint32_t>           PropertyNameSet;

    typedef std::vector< EntWeight > EntWeightVec;
    TopicMap d_topics;
    
    EntWeightVec d_vec;
    /// string ids of property names
    PropertyNameSet d_propNames;

    enum { 
         FILTER_MODE_LIGHT,  /// in light mode entities not filtered by anything will be displayed
         FILTER_MODE_STRICT  /// in strict mode only entities that resulted from filtering will be displayed
    };
    
    int d_topicFilterMode;

    BarzTopics() : d_topicFilterMode(FILTER_MODE_LIGHT) {}

    bool isTopicFilterMode_strict() const { return (d_topicFilterMode==FILTER_MODE_STRICT); }
    void setTopicFilterMode_Strict() 
        { if( d_topicFilterMode!= FILTER_MODE_STRICT) d_topicFilterMode= FILTER_MODE_STRICT; }
    void setTopicFilterMode_Light() 
        { if( d_topicFilterMode!= FILTER_MODE_LIGHT) d_topicFilterMode= FILTER_MODE_LIGHT; }

    const TopicMap& getTopicMap() const { return d_topics; }

    int getTopicWeight( const BarzerEntity& t ) const { 
        TopicMap::const_iterator i = d_topics.find(t);

        return ( i == d_topics.end() ? 0 : i->second );
    }
    enum { DEFAULT_TOPIC_WEIGHT=100, MIN_TOPIC_WEIGHT=1 }; 
    int addTopic( const BarzerEntity& t, int weight = DEFAULT_TOPIC_WEIGHT ) { 
        TopicMap::iterator i = d_topics.find(t);
        if( i == d_topics.end() ) 
            i = d_topics.insert( EntWeight(t,0) ).first;
        
        i->second += weight;
        return i->second;
    } 
    
    EntWeightVec& computeTopTopics( );
    void clear() {
        d_topics.clear();
        d_vec.clear();
    }
    bool hasTopics() const { return !d_topics.empty(); }
    void addPropName( uint32_t n ) { d_propNames.insert(n); }

    const PropertyNameSet& getPropNames() const { return d_propNames; }
};
inline bool operator<( const BarzTopics::EntWeight& l, const BarzTopics::EntWeight& r )
{ return ( l.second > r.second ); }


inline BarzTopics::EntWeightVec& BarzTopics::computeTopTopics( ) 
{
    if( !d_topics.empty() ) {
        for( BarzTopics::TopicMap::const_iterator i = d_topics.begin(); i!= d_topics.end(); ++i ) {
            d_vec.push_back( *i );
        }
        /// sorting by weight for processing 
        std::sort( d_vec.begin(), d_vec.end() );
        /// now we have a vector sorted in descending order 
        /// we will trim it 
      
        size_t newSize = 1;
        int weight = d_vec.front().second;
        for( ;newSize< d_vec.size(); ++newSize ) {
            if( d_vec[newSize].second < weight ) 
                break;
        }
        if( newSize< d_vec.size() ) {
            d_vec.resize( newSize );
            d_topics.clear();
            for( EntWeightVec::const_iterator i = d_vec.begin(); i!= d_vec.end(); ++i ) {
                d_topics.insert( *i );
            }
        }
    }
    return d_vec;
}

/// this object is stored with every "grammar" (pointer to trie) in the UniverseTrieCluster
class TrieTopics {
    BarzTopics::EntWeight d_entWeight;
public:    
    TrieTopics() : d_entWeight( BarzerEntity(), BarzTopics::DEFAULT_TOPIC_WEIGHT) {}

    void mustHave( const BarzerEntity& ent, int minWeight = BarzTopics::DEFAULT_TOPIC_WEIGHT )
    {
        d_entWeight.first = ent;
        d_entWeight.second = minWeight;
    }

    bool goodToGo( const BarzTopics& barzTopics ) const
    {
        if( !d_entWeight.first.eclass.isValid() ) 
            return true;
        else {
            const BarzerEntity& ent = d_entWeight.first;

            int w = barzTopics.getTopicWeight(ent);
            return( w>= d_entWeight.second ) ;
        }
        return false;
    }
};


struct GrammarInfo {
    TrieTopics trieTopics;
    bool  d_autocApplies;

    bool goodToGo( const BarzTopics& barzTopics ) const
        { return trieTopics.goodToGo(barzTopics); }

    GrammarInfo() : d_autocApplies(true) {} 

    void set_autocDontApply() { d_autocApplies= false; }
    bool autocApplies() const { return d_autocApplies; }
};
class TheGrammar {
    BELTrie* d_trie;
    GrammarInfo* d_grammarInfo;
    
    /// cant create trie-less grammar
    TheGrammar( ) : d_trie(0), d_grammarInfo(0) {}
public:

    TheGrammar( BELTrie* t, GrammarInfo* gi= 0) :
        d_trie(t), 
        d_grammarInfo(gi)
    {}
    
    const BELTrie*  triePtr() const { return d_trie; }
    BELTrie* triePtr() { return d_trie; }
    const BELTrie&  trie() const { return *d_trie; }
    BELTrie& trie() { return *d_trie; }
    
    const GrammarInfo* grammarInfo() const { return d_grammarInfo; } 
    GrammarInfo* grammarInfo() { return d_grammarInfo; } 
    
    GrammarInfo& setGrammarInfo( GrammarInfo* gi ) {
        return( d_grammarInfo = gi, *d_grammarInfo );
    }
}; 
/// entity segregator is important for post semantical phase 
/// it holds information used for grouping
class EntitySegregatorData {
    std::set< uint32_t > d_classToSeg; // entity classes to segregate
public:
    bool empty() const { return d_classToSeg.empty(); }

    void add( const StoredEntityClass& ec ) { d_classToSeg.insert( ec.ec ); }
    bool needSegregate( const StoredEntityClass& ec ) const 
        { return ( d_classToSeg.find(ec.ec) != d_classToSeg.end() ); }
};
//// topic entity linkage object is responsible for relations between different 
//// entities
struct TopicEntLinkage {
    typedef std::set< BarzerEntity > BarzerEntitySet;
    typedef std::map< BarzerEntity, BarzerEntitySet  > BarzerEntitySetMap;
    BarzerEntitySetMap topicToEnt;
    
    void link( const BarzerEntity& t, const BarzerEntity& e ) 
    {
        BarzerEntitySetMap::iterator i = topicToEnt.find( t );
        if( i == topicToEnt.end() ) {
            i = topicToEnt.insert( BarzerEntitySetMap::value_type( t, BarzerEntitySet() ) ).first; 
        }
        i->second.insert( e );
    }
    
    const BarzerEntitySet* getTopicEntities( const BarzerEntity& t ) const
    {
        BarzerEntitySetMap::const_iterator i = topicToEnt.find( t );
        return( i == topicToEnt.end() ? 0: &(i->second) );
    }

    void append( const TopicEntLinkage& lkg ) 
    {
        for( BarzerEntitySetMap::const_iterator i = lkg.topicToEnt.begin(); i!= lkg.topicToEnt.end(); ++i ) {
            const BarzerEntity& topic = i->first;
            const BarzerEntitySet& linkedSet = i->second;
            for( BarzerEntitySet::const_iterator lei = linkedSet.begin(); lei != linkedSet.end(); ++lei ) {
                link( topic, *lei );
            }
        }
    }

    void clear() { topicToEnt.clear(); }
}; // TopicEntLinkage

} // namespace barzer
#endif // BARZER_TOPICS_H
