
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#include "barzer_ruleidx.h"
#include "barzer_el_trie.h"

namespace barzer
{
void RuleIdx::addNode(const RulePath& path, BarzelTrieNode *node, uint32_t position)
{
	{
		auto pos = m_path2nodes.find(path);
		if (pos == m_path2nodes.end())
			pos = m_path2nodes.insert({ path, std::vector<detail::CountedPosInfo>() }).first;

		auto& vec = pos->second;
		const auto nodePos  = std::find_if(vec.begin(), vec.end(),
				[node, position](const detail::CountedPosInfo& info)
					{ return info.m_node == node && info.m_pos == position; });
		if (nodePos  == vec.end())
			vec.push_back({ node, position });
		else
			++nodePos->m_refcount;
	}

	{
		auto pos = m_node2rules.find(node);
		if (pos == m_node2rules.end())
			pos = m_node2rules.insert({ node, std::vector<RulePath>() }).first;

		pos->second.push_back(path);
	}
}

void RuleIdx::removeNode (const RulePath& path)
{
	const auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		return;				// TODO shit bricks

	for (auto& info : pos->second)
	{
		if (--info.m_refcount)
			continue;

		handleRuleRemovedFrom(path, info);

		BarzelTrieNode *node = info.m_node;
		// node removal algo goes here
	}

	m_path2nodes.erase(pos);
}

void RuleIdx::handleRuleRemovedFrom (const RulePath& path, const detail::CountedPosInfo& thisInfo)
{
	const auto pos = m_node2rules.find(thisInfo.m_node);
	if (pos == m_node2rules.end())
		return;											// TODO shit bricks ×2

	auto& vec = pos->second;
	const auto thisPos = std::find(vec.begin(), vec.end(), path);
	if (thisPos == vec.end())
		return;											// TODO shit bricks ×2

	vec.erase(thisPos);

	for (const auto& otherRule : vec)
	{
		const auto otherPos = m_path2nodes.find(otherRule);
		if (otherPos == m_path2nodes.end())
			continue;									// TODO shit bricks

		for (auto& otherInfo : otherPos->second)
		{
			if (otherInfo.m_node != thisInfo.m_node)
				continue;
			
			/* Update all other infos - decrement refcount if the position is
			 * same whithin the trie node or decrement the position index if it
			 * was greater.
			 * 
			 * Refcount will still be positive after the removal since the
			 * \em otherRule still references this position.
			 */
			if (otherInfo.m_pos == thisInfo.m_pos)
				--otherInfo.m_refcount;
			else if (otherInfo.m_pos > thisInfo.m_pos)
				--otherInfo.m_pos;
		}
	}
}
}
