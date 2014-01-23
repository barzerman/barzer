#pragma once
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
    // property names in zero universe (if any)
    PropertyNameSet d_zeroUniversePropNames;

    enum { 
         FILTER_MODE_LIGHT,  /// in light mode entities not filtered by anything will be displayed
         FILTER_MODE_STRICT  /// in strict mode only entities that resulted from filtering will be displayed
    };
    
    int d_topicFilterMode;

    bool hasZeroUniverseProperties() const { return d_zeroUniversePropNames.size(); }
    bool hasProperties() const { return(d_propNames.size() || d_zeroUniversePropNames.size()); }
    BarzTopics() : d_topicFilterMode(FILTER_MODE_LIGHT) {}

    bool isTopicFilterMode_strict() const { return (d_topicFilterMode==FILTER_MODE_STRICT); }
    void setTopicFilterMode_Strict() 
        { if( d_topicFilterMode!= FILTER_MODE_STRICT) d_topicFilterMode= FILTER_MODE_STRICT; }
    void setTopicFilterMode_Light() 
        { if( d_topicFilterMode!= FILTER_MODE_LIGHT) d_topicFilterMode= FILTER_MODE_LIGHT; }

    const TopicMap& getTopicMap() const { return d_topics; }

    int getTopicWeight( const BarzerEntity& t, bool& hasTopic ) const { 
        TopicMap::const_iterator i = d_topics.find(t);
        
        return ( i == d_topics.end() ? 0 : (hasTopic=true,i->second) );
    }
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
        d_propNames.clear();
        d_zeroUniversePropNames.clear();
    }
    bool hasTopics() const { return !d_topics.empty(); }
    void addPropName( uint32_t n ) { d_propNames.insert(n); }
    void addPropNameZeroUniverse( uint32_t n ) { d_zeroUniversePropNames.insert(n); }

    const PropertyNameSet& getPropNames() const { return d_propNames; }
    const PropertyNameSet& getPropNamesZeroUniverse() const { return d_zeroUniversePropNames; }
};
inline bool operator<( const BarzTopics::EntWeight& l, const BarzTopics::EntWeight& r )
    { return ( l.second > r.second ); }

/// this object is stored with every "grammar" (pointer to trie) in the UniverseTrieCluster
class TrieTopics {
    BarzTopics::EntWeight d_entWeight;
public:    
    TrieTopics() : d_entWeight( BarzerEntity(), BarzTopics::DEFAULT_TOPIC_WEIGHT) {}

    void mustHave( const BarzerEntity& ent, int minWeight = BarzTopics::DEFAULT_TOPIC_WEIGHT );
    bool goodToGo( const BarzTopics& barzTopics ) const;
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
    
    GrammarInfo& setGrammarInfo( GrammarInfo* gi ) { return( d_grammarInfo = gi, *d_grammarInfo ); }
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
    void clear() { d_classToSeg.clear(); }
};
//// topic entity linkage object is responsible for relations between different 
//// entities. stored with every Universe 
struct TopicEntLinkage {
    struct EntityData {
        BarzerEntity ent;
        uint32_t     strength;
        EntityData( const BarzerEntity& e, uint32_t s=0 ) : ent(e), strength(s) {}
    };
    struct EntityData_comp_less { bool operator()( const EntityData& l, const EntityData& r ) const { return( l.ent< r.ent ); } };

    typedef std::set< EntityData, EntityData_comp_less > BarzerEntitySet;
    typedef std::map< BarzerEntity, BarzerEntitySet  > BarzerEntitySetMap;

    BarzerEntitySetMap topicToEnt;

    /// {topicType to entityType} map - all existing type pares (topic,entity) stored here 
    std::vector< std::pair<StoredEntityClass,StoredEntityClass> > d_filterTypePairs;
    
    void updateFilterTypePairs( const StoredEntityClass& topicClass, const StoredEntityClass& entClass );
    bool topicTypeAppliesToEntClass( const StoredEntityClass& topicClass, const StoredEntityClass& entClass ) const;

    void link( const BarzerEntity& t, const EntityData& e );
    void link( const BarzerEntity& t, const BarzerEntity& e, uint32_t strength ) ;

    const BarzerEntitySet* getTopicEntities( const BarzerEntity& t ) const
    {
        BarzerEntitySetMap::const_iterator i = topicToEnt.find( t );
        return( i == topicToEnt.end() ? 0: &(i->second) );
    }

    void append( const TopicEntLinkage& lkg ) ;
    void clear();
}; // TopicEntLinkage

/// object filtering for a given list of topics
/// usage: 
///     - addFilteredEntClass() for all entities in the result set
///     - addTopic() for all topics
///     - optimize() - to order filters in an optimal way
class TopicFilter {
    const TopicEntLinkage& d_topicLinks;
    typedef std::vector<std::pair<BarzerEntity,const TopicEntLinkage::BarzerEntitySet*>> Ent2EntSetVec;
    std::vector< Ent2EntSetVec > d_filterVec;

    std::vector< StoredEntityClass > d_filteredEntClasses; 
public:
    TopicFilter( const TopicEntLinkage& linkage ) : d_topicLinks(linkage) {}

    bool addFilteredEntClass( const StoredEntityClass& ec );
    bool isFilterApplicable( const StoredEntityClass& ec ) const ;
    void clear();
    bool addTopic( const BarzerEntity& t );
    void optimize();
    bool isEntityGood( const BarzerEntity& e ) const ;
};

} // namespace barzer
