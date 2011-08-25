#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <ay/ay_choose.h>

namespace barzer {

void BZSpell::addExtraWordToDictionary( uint32_t strId, uint32_t frequency )
{
	BZSWordInfo& wi = d_wordinfoMap[ strId ];
	if( wi.upgradePriority( BZSWordInfo::WPRI_USER_EXTRA ) )
		wi.setFrequency( frequency );
}

/// when fails 0xffffffff is returned 
uint32_t BZSpell::getSpellCorrection( const char* s ) const
{
	return 0xffffffff;
}
uint32_t BZSpell::getStem( const char* s ) const
{
	return 0xffffffff;
}

namespace {

struct WPCallback {
	BZSpell& bzs;
	uint32_t fullStrId;
	size_t   varCount; 
	WPCallback( BZSpell& b, uint32_t l2  ) : bzs(b), fullStrId(l2), varCount(0)  {}
	typedef std::vector<char> charvec;
	typedef charvec::const_iterator charvec_ci;

	int operator()( charvec_ci fromI, charvec_ci toI )
	{
		charvec v( fromI, toI );
		v.push_back(0);
		GlobalPools& gp = bzs.getUniverse().getGlobalPools();
		const char* str = &(v[0]);

		uint32_t strId = gp.string_intern( str );

		bzs.addWordToLinkedWordsMap( strId, fullStrId );
		++varCount;
		return 0;
	}
};

}

size_t BZSpell::produceWordVariants( uint32_t strId )
{
	/// for ascii 
	const char* str = d_universe.getGlobalPools().string_resolve( strId );
	if( !str ) 
		return 0;

	size_t str_len = strlen( str );
	WPCallback cb( *this, strId );	
	ay::choose_n<char, WPCallback > variator( cb, str_len-1, str_len-1 );
	variator( str, str+str_len );
	return cb.varCount;
}

size_t BZSpell::init( const StoredUniverse* secondaryUniverse ) 
{
	if( secondaryUniverse ) {
		d_secondarySpellchecker = secondaryUniverse->getBZSpell();
	} else {
		const UniverseTrieCluster::BELTrieList& trieList = d_universe.getTrieList(); 
		for( UniverseTrieCluster::BELTrieList::const_iterator t = trieList.begin(); t!= trieList.end(); ++t ) {
			const strid_to_triewordinfo_map& wiMap = (*t)->getWordInfoMap();
			for( strid_to_triewordinfo_map::const_iterator w = wiMap.begin(); w != wiMap.end(); ++w ) {
				const TrieWordInfo& wordInfo = w->second;
				uint32_t strId = w->first;
	
				BZSWordInfo& wi = d_wordinfoMap[ strId ];
				
				if( wi.upgradePriority( (*t)->getSpellPriority()) )
					wi.setFrequency( wordInfo.wordCount );
			}
		}
	}

	/// at this point d_wordinfoMap has an entry for every word 
	/// now we will generate edit distance 1 variants for each word 
	for( strid_wordinfo_hmap::const_iterator wi = d_wordinfoMap.begin(); wi!= d_wordinfoMap.end(); ++wi ) {
		uint32_t strId = wi->first;

		produceWordVariants( strId );
	}
	return d_linkedWordsMap.size();
}

} // namespace barzer
