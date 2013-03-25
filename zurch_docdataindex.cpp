
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 

#include "zurch_docdataindex.h"

namespace zurch
{
void DocDataIndex::addInfo(uint32_t docId, const DocInfo::value_type& value)
{
	auto docPos = m_index.find(docId);
	if (docPos == m_index.end())
	{
		m_index.insert({ docId, { value } }).first;
		return;
	}
	
	docPos->second.insert(value);
}

bool DocDataIndex::operator()(uint32_t docId, const Filters::Filter_t& f) const
{
	auto docPos = m_index.find(docId);
	if (docPos == m_index.end())
		return false;
	
	const auto& map = docPos->second;
	return boost::apply_visitor(Filters::FilterTApplier(map), f);
}
}
