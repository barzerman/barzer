
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#pragma once

#include <vector>
#include <map>

namespace barzer
{
class BarzelTrieNode;

struct RulePath
{
    typedef std::pair< uint32_t, uint32_t > TrieId;
    TrieId m_trieId;

	uint32_t m_source;
	uint32_t m_stId;

    RulePath( ) : 
        m_trieId( { 0xffffffff, 0xffffffff } ),
        m_source(0xffffffff),m_stId(0xffffffff)
    {}
    RulePath( uint32_t srcid, uint32_t stid, uint32_t trieClassId, uint32_t trieId  ) : 
        m_trieId( { trieClassId, trieId } ),
        m_source(srcid),m_stId(stid)
    {}
    RulePath( uint32_t srcid, uint32_t stid, const TrieId& tid ) :
        m_trieId(tid),
        m_source(srcid), m_stId(stid)
    {}
};

inline bool operator<(const RulePath::TrieId& l, const RulePath::TrieId& r)
    { return( l.first < r.first ? true : ( r.first < l.first ? false : (l.second < r.second) )); }

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
	EList,		// the default, seems like anything is potentially an entlist (or won't be added anyway)
	Subtree
};

namespace detail
{
	struct PosInfo
	{
		BarzelTrieNode *m_node;
		uint32_t m_pos;
		NodeType m_type;
		
		PosInfo(BarzelTrieNode *n = 0, NodeType t = NodeType::EList, size_t p = 0)
		: m_node(n)
		, m_pos(p)
		, m_type(t)
		{ }
	};

	inline bool operator==(const PosInfo& p1, const PosInfo& p2)
	{
		return p1.m_node == p2.m_node && p1.m_pos == p2.m_pos;
	}

}

class TrieRuleIdx {
	std::map<RulePath, std::vector<detail::PosInfo>> m_path2nodes;
public:
	/** @brief Adds the given node as associated with the rule identified by path.
	 *
	 * @param[in] path The path of the rule as the source ID and statement ID.
	 * @param[in] node The node modified as the result of the rule parsing.
	 * @param[in] position The position of an entity (or smth) in the entlist (or
	 * smth) in the \em node.
	 */
	void addNode(const RulePath& path, BarzelTrieNode *node, uint32_t position, NodeType type);

	void removeNode(const RulePath& path);
     
    void clear();
};

}
