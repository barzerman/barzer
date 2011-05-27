#ifndef BARZER_EL_ANALYSIS_H
#define BARZER_EL_ANALYSIS_H

#include <barzer_el_btnd.h>
#include <barzer_el_parser.h>
namespace barzer {


/// data analysis trie is used to analyse the dataset only. it's a different object
/// from BELTrie and regular matching doesnt work on it. It's basically just a trie on
/// tokens . The right side is expecte to be an entity 
struct DATrie {
	const BarzelTrieNode* addAnalyticalStatement( const BELStatementParsed& stmt, const BTND_PatternDataVec& path );
};

} // barzer namespace 
#endif // BARZER_EL_ANALYSIS_H
