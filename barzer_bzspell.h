#ifndef BARZER_BZSPELL_H 
#define BARZER_BZSPELL_H 

#include <ay_evovec.h>
#include <boost/unordered_map.hpp>

namespace barzer {

/// for every word visible by the user this is stored int he universe
struct BZSWordInfo {
	// 0 - generic fluff 
	// 1 - user specific extra words 
	// 5 - generic non fluff
	// 10 - user specific fluf
	// 15 - user specific non fluff
	// ... the higher the number the "heavier" the word is and the likely it is to be selected 
	// by the spellchecker 
	uint8_t d_priority;

	/// frequency of the given word among the user-visible words
	// 1-100 - the higher the score the heavier the word - this is the 2-nd order factor
	/// initially this may contain occurence counts
	uint32_t d_freq;
	enum {
		WPRI_GENERIC_FLUFF = 0,
		WPRI_USER_EXTRA = 1,
		WPRI_GENERIC_NONFLUFF = 5,
		WPRI_USER_FLUFF = 10,
		WPRI_USER_NONFLUFF = 15
	};

	void setFrequency( uint32_t f ) { d_freq= f; }
	bool upgradePriority( uint8_t p ) 
	{
		if( p> d_priority ) {
			d_priority= p;
			return true;
		} else 
			return false;
	}
	uint32_t getPriority() const { return d_priority; }
	uint32_t getFrequency() const { return d_freq; }
};

/// one of thes eis stored for each unique strId in a trie 
struct BZSWordTrieInfo {
	uint32_t wordCount; 
	
	BZSWordTrieInfo() : wordCount(0) {}
	void incrementCount() { ++wordCount; }
};

class BZSpell {
	StoredUniverse& d_universe;
public:
	/// the evos must be ordered by priority + frequency in descending order 
	typedef boost::unordered_map< uint32_t, ay::evovec_uint32 > strid_evovec_hmap;
	typedef boost::unordered_map< uint32_t, BZSWordInfo > strid_wordinfo_hmap;

	/// the next spell checker in line  
	const BZSpell* d_secondarySpellchecker; 
private: 
	strid_wordinfo_hmap d_wordinfoMap;	// from actual words to universe specific word info 
	strid_evovec_hmap  	d_linkedWordsMap;  // words linked to partial word
	
	std::string d_extraWordsFileName;

	/// generates edit distance variants 
	size_t produceWordVariants( uint32_t strId ); 
public:
	void addWordToLinkedWordsMap(uint32_t linkTo, uint32_t strId )
	{
		d_linkedWordsMap[ linkTo ].push_back( strId );
	}
	StoredUniverse& getUniverse() { return d_universe; }
	const StoredUniverse& getUniverse() const { return d_universe; }
	void clear()
	{
		d_wordinfoMap.clear();
		d_linkedWordsMap.clear();
	}
	void addExtraWordToDictionary( uint32_t, uint32_t frequency = 0 );

	BZSpell( StoredUniverse& uni ) : d_universe( uni ) {}

	// returns the size of strid_evovec_hmap
	// this function should be called after the load. 
	// specifically: after the last addExtraWordToDictionary has been called
	size_t init( const StoredUniverse* secondaryUniverse =0 );

	/// when fails 0xffffffff is returned 
	uint32_t getSpellCorrection( const char* s ) const;
	uint32_t getStem( const char* s ) const;

	void loadExtra( const char* fileName );
};

}

#endif // BARZER_BZSPELL_H 
