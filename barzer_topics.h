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

    typedef std::vector< EntWeight > EntWeightVec;
    TopicMap d_topics;
    
    EntWeightVec d_vec;
public:
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
    bool goodToGo( const BarzTopics& barzTopics ) const
    { return trieTopics.goodToGo(barzTopics); }
    
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

} // namespace barzer
#endif // BARZER_TOPICS_H
