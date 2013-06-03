
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#include "barzer_ruleidx.h"
#include "barzer_el_trie.h"

namespace barzer
{
void RuleIdx::addNode(const RulePath& path, BarzelTrieNode *node, uint32_t position)
{
	auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		pos = m_path2nodes.insert({ path, std::vector<detail::PosInfo>() }).first;

	auto& vec = pos->second;
	const auto nodePos = std::find_if(vec.begin(), vec.end(),
			[node, position](const detail::PosInfo& info)
				{ return info.m_node == node && info.m_pos == position; });
	if (nodePos  == vec.end())
		vec.push_back({ node, detail::PosInfo::Type::EList, position });
	node->ref();
}

void RuleIdx::removeNode (const RulePath& path)
{
	const auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		return;				// TODO shit bricks

	for (auto& info : pos->second)
	{
		if (info.m_node->deref())
			continue;

		BarzelTrieNode *node = info.m_node;
		// node removal algo goes here
	}

	m_path2nodes.erase(pos);
}
}
