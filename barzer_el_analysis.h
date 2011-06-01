#ifndef BARZER_EL_ANALYSIS_H
#define BARZER_EL_ANALYSIS_H

#include <barzer_universe.h>
#include <barzer_el_btnd.h>
#include <barzer_el_trie_walker.h>
#include <ay/ay_vector.h>
#include <ay/ay_util_char.h>

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
	std::ostream& print( std::ostream& fp, const StoredUniverse& u ) const;
};

struct TrieAnalyzer {
	const StoredUniverse& d_universe;
	BarzelTrieTraverser_depth d_trav;
	typedef const BarzelTrieNode* BTN_cp;
	typedef boost::unordered_map< BTN_cp, TA_BTN_data > BTNDataHash;
	BTNDataHash d_dtaHash;
	
	size_t d_nameThreshold;   // if < d_nameThreshold - qualifies as a name
	size_t d_fluffThreshold;  // if > d_fluffThreshold - can be considered fluff
	
	size_t getNameThreshold() const { return d_nameThreshold; }
	size_t getFluffThreshold() const { return d_fluffThreshold; }
	TrieAnalyzer( const StoredUniverse& );
	
	void setNameThreshold( size_t n );
	void setFluffThreshold( size_t n ) ;

	const TA_BTN_data* getTrieNodeData( BTN_cp tn ) const 
	{
		BTNDataHash::const_iterator i = d_dtaHash.find( tn );
		return( i == d_dtaHash.end() ? 0 : &(i->second) );
	}
	TA_BTN_data* getTrieNodeData( BTN_cp tn )
	{ 
		BTNDataHash::iterator i = d_dtaHash.find( tn );
		return( i == d_dtaHash.end() ? 0 : &(i->second) );
	}
	
	const StoredEntity* getEntityById( uint32_t id ) const
	{
		return getUniverse().getDtaIdx().getEntById( id );
	}
	const char* getIdStr( const StoredEntityUniqId& euid ) const
	{
		return getUniverse().getDtaIdx().resolveStoredTokenStr( euid.tokId ); 
	}
	const StoredUniverse& getUniverse() const { return d_universe; }
	BarzelTrieTraverser_depth& getTraverser() { return d_trav; }
	void updateAnalytics( BTN_cp, TA_BTN_data& dta );
	bool operator()( const BarzelTrieNode& t ) ;

	void traverse( ) 
		{ getTraverser().traverse( *this, getUniverse().getBarzelTrie().getRoot() ); }

	//// returns string values for the current tokens (vector of const char*)
	//// this only wors if the current traverser's path is made up entirely of the 
	//// firm keys (tokens as opposed to wildcards)
	//// when a non-token (wildcard) is encountered the effort is abprted and *false*
	//// is returned otherwise the function returns *true*
	bool getPathTokens( ay::char_cp_vec& tvec ) const ;

	std::ostream& print( std::ostream& fp ) const;
};

//// specific must have 
//// operator ( TrieAnalyzer& , const BarzelTrieNode& t ) defined . 
//// it will be called back for every node in the trie
template <typename T>
struct TrieAnalyzerTraverser {
	TrieAnalyzer& analyzer;
	T& specific;

	TrieAnalyzerTraverser( TrieAnalyzer& ta, T& s ) : 
		analyzer(ta), specific(s) {}

	bool operator()( const BarzelTrieNode& t ) 
		{ return specific( analyzer, t ); }
	
	void traverse( )
		{ analyzer.getTraverser().traverse( *this, analyzer.getUniverse().getBarzelTrie().getRoot() ); }
};

/// name [producer] - specific for analyzer traverser 
struct TANameProducer {
	size_t d_numNames, d_numFluff; // number of name and fluff patterns 
	std::ostream& d_fp;
	TANameProducer( std::ostream& fp ) : 
		d_numNames(0),
		d_numFluff(0),
		d_fp(fp)
	{}
	bool operator()( TrieAnalyzer& analyzer, const BarzelTrieNode& t ) ;
};

} // barzer namespace 
#endif // BARZER_EL_ANALYSIS_H
