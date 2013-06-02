
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#pragma once

#include <boost/unordered_map.hpp>

namespace barzer
{
class BarzelTrieNode;

struct RulePath
{
	uint32_t m_source;
	size_t m_stId;
};

inline bool operator==(const RulePath& r1, const RulePath& r2)
{
	return r1.m_source == r2.m_source && r1.m_stId == r2.m_stId;
}

inline uint64_t hash_value(const RulePath& r)
{
	return (static_cast<uint64_t>(r.m_source) << 32) + r.m_stId;
}

namespace detail
{
	struct PosInfo
	{
		BarzelTrieNode *m_node;
		size_t m_pos;

		PosInfo(BarzelTrieNode *n = 0, size_t p = 0)
		: m_node(n)
		, m_pos(p)
		{
		}
	};

	inline bool operator== (const PosInfo& p1, const PosInfo& p2)
	{
		return p1.m_node == p2.m_node && p1.m_pos == p2.m_pos;
	}

	inline uint64_t hash_value(const PosInfo& pi)
	{
		return pi.m_pos + boost::hash_value(pi.m_node);
	}

	struct CountedPosInfo : PosInfo
	{
		size_t m_refcount;

		CountedPosInfo(BarzelTrieNode *n = 0, size_t p = 0)
		: PosInfo(n, p)
		, m_refcount(1)
		{
		}
	};
}

class RuleIdx
{
	boost::unordered_map<RulePath, std::vector<detail::CountedPosInfo>> m_path2nodes;

	/** Inverted index, is useful to properly update positional info on rules removal.
	 */
	boost::unordered_map<BarzelTrieNode*, std::vector<RulePath>> m_node2rules;
public:
	/** @brief Adds the given node as associated with the rule identified by path.
	 *
	 * @param[in] path The path of the rule as the source ID and statement ID.
	 * @param[in] node The node modified as the result of the rule parsing.
	 * @param[in] position The position of an entity (or smth) in the entlist (or
	 * smth) in the \em node.
	 */
	void addNode(const RulePath& path, BarzelTrieNode *node, uint32_t position = 0);

	void removeNode(const RulePath& path);
private:
	void handleRuleRemovedFrom(const RulePath& path, const detail::CountedPosInfo& ruleInfo);
};
}
