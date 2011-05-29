#ifndef BARZER_EL_ANALYSIS_H
#define BARZER_EL_ANALYSIS_H

#include <barzer_universe.h>
#include <barzer_el_btnd.h>
#include <barzer_el_trie_walker.h>
namespace barzer {

// analytical data 
struct TA_BTN_data {
	uint32_t numDesc,  // number of descendants + 1 
			 numDescEntities, // number of entities in descendants 
		     numEntSelf;      // number of entities in self

	TA_BTN_data() : 
		numDesc(0), numDescEntities(0), numEntSelf(0)
	{}
};

struct TrieAnalyzer {
	const StoredUniverse& d_universe;
	BarzelTrieTraverser_depth d_trav;
	typedef const BarzelTrieNode* BTN_cp;
	typedef boost::unordered_map< BTN_cp, TA_BTN_data > BTNDataHash;
	BTNDataHash d_dtaHash;
	
	int d_nameThreshold;   // if < d_nameThreshold - qualifies as a name
	int d_fluffThreshold;  // if > d_fluffThreshold - can be considered fluff

	TrieAnalyzer( const StoredUniverse& u );
	
	TrieAnalyzer( const StoredUniverse& );

	BarzelTrieTraverser_depth& getTraverser() { return d_trav; }
	void updateAnalytics( BTN_cp, TA_BTN_data& dta );
	bool operator()( const BarzelTrieNode& t ) ;
};

} // barzer namespace 
#endif // BARZER_EL_ANALYSIS_H
