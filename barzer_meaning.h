#ifndef BARZER_MEANING_H
#define BARZER_MEANING_H

#include <boost/unordered_map.hpp>

#include <vector>

namespace barzer {
class StoredUniverse;
class GlobalPools;

struct WordMeaning {
    uint32_t id;
    uint8_t prio;

    WordMeaning() : id(0xffffffff), prio(0) {}
    WordMeaning(uint32_t i, uint8_t p=0) : id(i), prio(p) {}
};

typedef std::pair<const WordMeaning*, size_t> WordMeaningBufPtr;
typedef std::pair<const uint32_t*, size_t> MeaningSetBufPtr;

typedef std::vector<WordMeaning> MVec_t;
typedef std::vector<uint32_t> WVec_t;

typedef boost::unordered_map<uint32_t, MVec_t> W2MDict_t;
typedef boost::unordered_map<uint32_t, WVec_t> M2WDict_t;

/// stored in global pools 
class MeaningsStorage {
    W2MDict_t m_word2meanings;
    M2WDict_t m_meaning2words;
public:
    const W2MDict_t& getWordsToMeaningsDict() const { return m_word2meanings; }
    const M2WDict_t& getMeaningsToWordsDict() const { return m_meaning2words; }

    void addMeaning(uint32_t wordId, WordMeaning meaning) {
        m_word2meanings[wordId].push_back(meaning);
        m_meaning2words[meaning.id].push_back(wordId);
    }

    WordMeaningBufPtr getMeanings(uint32_t wordId) const {
        W2MDict_t::const_iterator pos = m_word2meanings.find(wordId);
        if( pos != m_word2meanings.end() && pos->second.size() ) {
           return WordMeaningBufPtr( &(pos->second[0]), pos->second.size() ); 
        } else
            return WordMeaningBufPtr(0,0);
    }
    
    MeaningSetBufPtr getWords(uint32_t meaningId) const {
        M2WDict_t::const_iterator pos = m_meaning2words.find(meaningId);
        if( pos != m_meaning2words.end() && pos->second.size()) {
            return MeaningSetBufPtr( &(pos->second[0]), pos->second.size());
        } else 
            return MeaningSetBufPtr(0, 0);
    }
};

// stored with each trie
class TrieMeanings {
    ///  list of meanings for each word applicable within the universe
    W2MDict_t m_w2m; 
public:
    const W2MDict_t& getW2M() const { return m_w2m; } 

    void addMeaning(uint32_t wordId, WordMeaning meaning) { m_w2m[wordId].push_back(meaning); }
    WordMeaningBufPtr getWordMeanings( uint32_t wordId ) const 
    {
        W2MDict_t::const_iterator pos = m_w2m.find(wordId);
        if( pos != m_w2m.end() ) {
            return WordMeaningBufPtr( &(pos->second[0]), pos->second.size() );
        } else {
            return WordMeaningBufPtr(0,0);
        }
    }
};

//// meanings XML parser 
struct MeaningsXMLParser {
    GlobalPools&    d_gp;
    StoredUniverse* d_universe;

    MeaningsXMLParser( GlobalPools& gp ) : d_gp(gp), d_universe(0){}

    std::vector< int > tagStack;

    /// tag functions are neede for external connectivity  so that this object can be invoked from other XML parsers
    void tagOpen( const char* tag, const char** attr, size_t attr_sz );
    void tagClose( const char* tag );
    void takeCData( const char* dta, size_t dta_len );

    bool isCurTag( int tid ) const { return ( tagStack.back() == tid ); }
    bool isParentTag( int tid ) const { return ( tagStack.size() > 1 && (*(tagStack.rbegin()+1)) == tid ); }

    void readFromFile( const char* fname );

    void clear() { tagStack.clear(); }
};

} // namespace barzer 

#endif //  BARZER_MEANING_H
