
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#include <barzer_el_trie_ruleidx.h>
#include <barzer_el_trie.h>
#include <barzer_universe.h>

namespace barzer
{
void TrieRuleIdx::addNode(const RulePath& path, BarzelTrieNode *node, uint32_t position, NodeType type)
{
	auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		pos = m_path2nodes.insert({ path, std::vector<detail::PosInfo>() }).first;

	auto& vec = pos->second;
	const auto nodePos = std::find_if(vec.begin(), vec.end(),
			[node, position](const detail::PosInfo& info)
				{ return info.m_node == node && info.m_pos == position; });
	if (nodePos  == vec.end())
		vec.push_back({ node, type, position });
	node->ref();
}

bool TrieRuleIdx::removeNode( const char* trieClass, const char* trieId , const char* source, uint32_t statementId )
{
    if( BELTrie* trie = d_universe.getTrieCluster().getTrieByClassAndId( trieClass, trieId ) ) {
        uint32_t sourceId = d_universe.getGlobalPools().internalString_getId( source );

        RulePath path( statementId, sourceId, d_universe.getTrieCluster().getUniqueTrieId(trieClass,trieId) );
        return removeNode(path) ;
    } else
        return false;
    
    return true;
}

bool TrieRuleIdx::removeNode (const RulePath& path)
{
	const auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		return false;

	for (auto& info : pos->second)
	{
        // info is detail::PosInfo (barzer_el_trie_ruleidx.h)

		if (info.m_node->deref())
			continue;

		auto node = info.m_node;
		switch (info.m_type)
		{
		case NodeType::EList:
			break;
		case NodeType::Subtree:
			break;
		}
	}

	m_path2nodes.erase(pos);
    return true;
}

} // namespace barzer 
