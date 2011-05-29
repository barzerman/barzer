#ifndef BARZER_EL_ANALYSIS_H
#define BARZER_EL_ANALYSIS_H

#include <barzer_universe.h>
#include <barzer_el_btnd.h>
#include <barzer_el_trie_walker.h>
#include <ay/ay_vector.h>
namespace barzer {

// analytical data 
struct TA_BTN_data {
	uint32_t numDesc;  // number of descendants + 1 
	typedef ay::vecset< uint32_t > EntVecSet;
	EntVecSet entities;
	
	void addEntity( uint32_t i ) { entities.insert(i); }

	TA_BTN_data() : 
		numDesc(0)
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

	
	TrieAnalyzer( const StoredUniverse& );

	BarzelTrieTraverser_depth& getTraverser() { return d_trav; }
	void updateAnalytics( BTN_cp, TA_BTN_data& dta );
	bool operator()( const BarzelTrieNode& t ) ;
};

} // barzer namespace 
#endif // BARZER_EL_ANALYSIS_H
