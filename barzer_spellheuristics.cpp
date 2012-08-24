#include <barzer_spellheuristics.h>
#include <barzer_universe.h>
#include <ay_translit_ru.h>

namespace barzer
{
	void SoundsLikeInfo::addSource(uint32_t soundsLike, uint32_t source)
	{
		SourceDictionary_t::iterator pos = m_sources.find(soundsLike);
		if (pos == m_sources.end()) 
			pos = m_sources.insert(std::make_pair(soundsLike, ay::StackVec<uint32_t>())).first;

        pos->second.push_back(source);
	}

	const SoundsLikeInfo::SourceList_t* SoundsLikeInfo::findSources(uint32_t like) const
	{
		SourceDictionary_t::const_iterator pos = m_sources.find(like);
		return pos == m_sources.end() ? 0 : &pos->second;
	}
	
	void HashingSpellHeuristic::addSource(const char *sourceWord, size_t len, uint32_t sourceId)
	{
		std::string out;
		transform(sourceWord, len, out);
		
		const uint32_t soundsId = m_gp.internString_internal(out.c_str(), out.size());
		m_mapping.addSource(soundsId, sourceId);
	}
	
	const SoundsLikeInfo::SourceList_t* HashingSpellHeuristic::findSources(const char *domainWord, size_t len) const
	{
		std::string out;
		transform(domainWord, len, out);
		
		const uint32_t id = m_gp.internalString_getId(out.c_str());
		return m_mapping.findSources(id);
	}
	
	void EnglishSLHeuristic::transform(const char *src, size_t srcLen, std::string& out) const
	{
		ay::tl::en2ru(src, srcLen, out);
	}
	
	void RuBastardizeHeuristic::transform(const char *src, size_t srcLen, std::string& out) const
	{
		char prev1 = 0;
		char prev2 = 0;
		for (const char *pos = src, *end = src + srcLen; pos < end; pos += 2)
		{
			if (ay::tl::is_not_russian_consonant(pos) || (pos[0] == prev1&& pos[1] == prev2) )
				continue;
			
			out.append(pos, 2);
			prev1 = pos[0];
			prev2 = pos[1];
		}
	}
	
	ChainHeuristic::ChainHeuristic(const HashingSpellHeuristic& in, const HashingSpellHeuristic& out)
	: HashingSpellHeuristic(in.getGP())
	, m_in(in)
	, m_out(out)
	{
	}
	
	void ChainHeuristic::transform(const char *src, size_t srcLen, std::string& out) const
	{
		std::string temp;
		m_in.transform(src, srcLen, temp);
		m_out.transform(temp.c_str(), temp.size(), out);
	}
}
