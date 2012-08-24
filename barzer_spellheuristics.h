#pragma once

#include <boost/unordered_map.hpp>
#include <ay/ay_stackvec.h>

namespace barzer
{
class GlobalPools;

class SoundsLikeInfo
{
public:
    typedef ay::StackVec<uint32_t> SourceList_t;
private:
    typedef boost::unordered_map<uint32_t, SourceList_t> SourceDictionary_t;
    SourceDictionary_t m_sources;
public:
    void addSource(uint32_t soundsLike, uint32_t source);
    const SourceList_t* findSources(uint32_t like) const;
};

/** A base class for heuristics that work by "dumbing" a word — transforming
 * it into some simpler form (like removing all vowels, for example), so that
 * each element from the transformation codomain can have multiple sources
 * from the transformation domain.
 */
class HashingSpellHeuristic
{
    GlobalPools& m_gp;
    SoundsLikeInfo m_mapping;
public:
    HashingSpellHeuristic(GlobalPools& g) : m_gp(g) {}
    virtual ~HashingSpellHeuristic() {}
    
    GlobalPools& getGP() const { return m_gp; }
    
    void addSource(const char *sourceWord, size_t len, uint32_t sourceId);
    const SoundsLikeInfo::SourceList_t* findSources(const char *domainWord, size_t len) const;
    
    virtual void transform(const char *src, size_t srcLen, std::string& out) const = 0;
};

class EnglishSLHeuristic : public HashingSpellHeuristic
{
public:
    EnglishSLHeuristic(GlobalPools& g) : HashingSpellHeuristic(g) {}
protected:
    void transform(const char* src, size_t srcLen, std::string& out) const;
};
/// this class 'barstardizes' russian text input in transform
/// 
class RuBastardizeHeuristic : public HashingSpellHeuristic
{
public:
    RuBastardizeHeuristic(GlobalPools& g) : HashingSpellHeuristic(g) {}
    
    void transform (const char* src, size_t srcLen, std::string& out) const;
};

class ChainHeuristic : public HashingSpellHeuristic
{
    const HashingSpellHeuristic& m_in;
    const HashingSpellHeuristic& m_out;
public:
    ChainHeuristic (const HashingSpellHeuristic&, const HashingSpellHeuristic&);
    
    void transform (const char* src, size_t srcLen, std::string& out) const;
};
} //namespace barzer
