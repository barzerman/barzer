
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#pragma once

#include <barzer_elementary_types.h>
#include <vector>
#include <map>

namespace barzer
{
class BarzelTrieNode;
class StoredUniverse;

struct RulePath
{
    UniqueTrieId m_trieId;

	uint32_t m_source;
	uint32_t m_stId;

    uint32_t source() const { return m_source; }
    uint32_t statementId() const { return m_stId; }
    void setTrieId( uint32_t cid, uint32_t tid ) 
        { m_trieId = { cid, tid }; }

    RulePath( ) : 
        m_trieId( { 0xffffffff, 0xffffffff } ),
        m_source(0xffffffff),m_stId(0xffffffff)
    {}
    RulePath( uint32_t srcid, uint32_t stid, uint32_t trieClassId, uint32_t trieId  ) : 
        m_trieId( { trieClassId, trieId } ),
        m_source(srcid),m_stId(stid)
    {}
    RulePath( uint32_t srcid, uint32_t stid, const UniqueTrieId& tid ) :
        m_trieId(tid),
        m_source(srcid), m_stId(stid)
    {}
};

inline bool operator<(const RulePath& l, const RulePath& r)
{
    return( l.m_stId < r.m_stId ? true :
        ( r.m_stId < l.m_stId ? false :
            ( l.m_source < r.m_source ? true :
                ( r.m_source < l.m_source ? true :
                    ( l.m_trieId < r.m_trieId )
                )
            )
            
        )
    );
}

inline bool operator==(const RulePath& r1, const RulePath& r2)
{
	return ( r1.m_source == r2.m_source && r1.m_stId == r2.m_stId );
}

enum class NodeType : uint8_t
{
	EList,		
	Subtree
};

class TrieRuleIdx {
public:
    typedef std::vector<BarzelTrieNode*> TrieNodeInfo;
private:
	std::map<RulePath, TrieNodeInfo> m_path2nodes;
    StoredUniverse& d_universe;
public:
	void addNode(const RulePath& path, BarzelTrieNode *node );

	bool removeNode(const RulePath& path);
    
    bool removeNode( 
        const char* trieClass, const char* trieId , 
        const char* source, 
        uint32_t statementId 
    );

    void clear();
    TrieRuleIdx( StoredUniverse& u) : d_universe(u) {}
};

}
