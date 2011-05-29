#include <barzer_el_analysis.h>
#include <barzer_el_trie.h>

namespace barzer {

TrieAnalyzer::TrieAnalyzer( const StoredUniverse& u ) :
	d_universe(u),
	d_trav( u.getBarzelTrie().getRoot(), u.getBarzelTrie() ),
	d_nameThreshold(1+u.getDtaIdx().getNumberOfEntities()/1000),
	d_fluffThreshold(1+u.getDtaIdx().getNumberOfEntities()/20)
{}

void TrieAnalyzer::updateAnalytics( BTN_cp tn, TA_BTN_data& dta )
{
	if( tn->isLeaf() ) {
		const BELTrie& trie = d_universe.getBarzelTrie();
		const BarzelTranslation* tran = trie.getBarzelTranslation( *tn );
		if( tran ) {
			if( tran->isMkEntSingle() ) {
				dta.addEntity( tran->getId_uint32() );
			} else if( tran->isMkEntList() ) {
				const EntityGroup* entGrp = trie.getEntityCollection().getEntGroup( tran->getId_uint32() );
				if( entGrp ) 
				dta.entities.insert( entGrp->getVec().begin(), entGrp->getVec().end());
			}
		}
	}
}

bool TrieAnalyzer::operator()( const BarzelTrieNode& t ) 
{
	BTN_cp tn = &t;
	BTNDataHash::iterator i = d_dtaHash.find( tn );
	if( i == d_dtaHash.end() ) {
		i = d_dtaHash.insert( BTNDataHash::value_type( tn, TA_BTN_data() ) ).first;
	}
	updateAnalytics( tn, i->second );
	return true;
}

} // namespace barzer
 
