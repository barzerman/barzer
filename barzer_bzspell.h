#ifndef BARZER_BZSPELL_H 
#define BARZER_BZSPELL_H 

#include <ay_evovec.h>
#include <boost/unordered_map.hpp>
#include <barzer_language.h>

namespace barzer {
class StoredUniverse;

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
    int16_t d_lang; // language code (LANG_XXX) - see barzer_language.h 

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
    
    BZSWordInfo() : 
        d_priority(WPRI_GENERIC_FLUFF),
        d_lang(LANG_ENGLISH),
        d_freq(0)
    {}
	void setFrequency( uint32_t f ) { d_freq= f; }
	bool upgradePriority( uint8_t p ) 
	{
		if( p> d_priority ) {
			d_priority= p;
			return true;
		} else 
			return false;
	}
    void    setLang( int16_t lang ) { d_lang = lang; }
	int     getLang() const { return d_lang; }

    bool isLang_English() const { return ( d_lang == LANG_ENGLISH); }
    bool isLang_Russian() const { return ( d_lang == LANG_RUSSIAN); }

	uint32_t getPriority() const { return d_priority; }
	uint32_t getFrequency() const { return d_freq; }

	bool lessThan( const BZSWordInfo& o ) const  {
		return ay::range_comp().less_than(
			d_priority, d_freq,
			o.d_priority, o.d_freq
		);
	}
};
inline bool operator <( const BZSWordInfo& l, const BZSWordInfo& r )
{ return l.lessThan( r ); }

/// one of thes eis stored for each unique strId in a trie 
struct BZSWordTrieInfo {
	uint32_t wordCount; 
	
	BZSWordTrieInfo() : wordCount(0) {}
	void incrementCount() { ++wordCount; }
	bool lessThan( const BZSWordTrieInfo& o ) const
	{
		return wordCount< o.wordCount; 
	}
};

class BZSpell {
	/// the next spell checker in line  
	const BZSpell* d_secondarySpellchecker; 
	StoredUniverse& d_universe;
public:

	/// the evos must be ordered by priority + frequency in descending order 
	// ay::evovec_uint32 is in ay/ay_evovec.h - it is a wrapper which is either a vector 
	// or a stack allocated contiguous storage
	typedef boost::unordered_map< uint32_t, ay::evovec_uint32 > strid_evovec_hmap;
	typedef boost::unordered_map< uint32_t, BZSWordInfo > strid_wordinfo_hmap;
	typedef std::pair<const BZSWordInfo*, size_t > WordInfoAndDepth;
    typedef std::map< ay::char_cp, uint32_t, ay::char_cp_compare_less > char_cp_to_strid_map;


	/// sizeof single character - 1 by default for ascii  
	uint8_t d_charSize; 

	size_t  d_minWordLengthToCorrect;

	enum { MAX_WORD_LEN = 128 };
private: 
	strid_wordinfo_hmap d_wordinfoMap;	// from actual words to universe specific word info 
	strid_evovec_hmap  	d_linkedWordsMap;  // words linked to partial word
    char_cp_to_strid_map    d_validTokenMap;
	
	std::string d_extraWordsFileName;

	/// generates edit distance variants 
	size_t produceWordVariants( uint32_t strId, int lang=LANG_ENGLISH ); 
public:

    const char_cp_to_strid_map* getValidWordMapPtr() const { return &d_validTokenMap; }
    const char_cp_to_strid_map& getValidWordMap() const { return d_validTokenMap; }

    const BZSWordInfo* getWordInfo( uint32_t id ) const
    {
        strid_wordinfo_hmap::const_iterator i = d_wordinfoMap.find( id );
        return ( i == d_wordinfoMap.end() ? 0 : &(i->second) );
    }
	bool isAscii() const { return ( d_charSize== 1 ); }
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

	BZSpell( StoredUniverse& uni ) ;
	~BZSpell();

	// returns the size of strid_evovec_hmap
	// this function should be called after the load. 
	// specifically: after the last addExtraWordToDictionary has been called
	size_t init( const StoredUniverse* secondaryUniverse =0 );

	/// when fails 0xffffffff is returned 
	uint32_t getSpellCorrection( const char* s, bool doStemCorrect, int lang=LANG_UNKNOWN ) const;
	uint32_t getStemCorrection( std::string& , const char*, int lang=LANG_UNKNOWN) const;
    // when bool doStemCorrect is false it will NOT correct to a string whose token is 
    // pure stem
    // extNorm - normalized string can be actually passed in
	uint32_t get2ByteLangStemCorrection( int lang, const char* str, bool doStemCorrect, const char* extNorm = 0 ) const;
	uint32_t getUtf8LangStemCorrection( int lang, const char* str, bool doStemCorrect, const char* extNorm = 0 ) const;
    uint32_t purePermuteCorrect(const char* s, size_t s_len )  const;

	uint32_t getAggressiveStem( std::string& , const char*) const;

	/// if word isnt found at all returns 0
	/// 1 - means it's the users word 
	/// >1 - secondary 
	// strId will have stringId or 0xffffffff if string cant be resolved
	int isUsersWord( uint32_t& strId, const char* word ) const ;
	int isUsersWordById( uint32_t ) const;

    bool isPureStem( const char* str ) const;
    bool isPureStem( uint32_t strId ) const;

	/// stems (currently only de-pluralizes) word . returns true if stemming was 
	/// successful
	bool     stem( std::string& out, const char* word ) const;
	bool     stem( std::string& out, const char* word, int& lang ) const;

	size_t loadExtra( const char* fileName );
	std::ostream& printStats( std::ostream& fp ) const;

	bool isWordValidInUniverse( const char* word ) const;

    bool isWordValidInUniverse( uint32_t  strId ) const
        { return ( strId == 0xffffffff ? false: (d_wordinfoMap.find( strId ) != d_wordinfoMap.end()) ) ; }


	// for each primary spellchecker depth gets incremented by 1 
	// returns string id of the best match
	uint32_t getBestWord( uint32_t strId, WordInfoAndDepth& wid ) const;

	uint32_t getBestWordByString( const char* word, WordInfoAndDepth& wid ) const;
	void setSecondarySpellchecker( const BZSpell* bzs ) { d_secondarySpellchecker= bzs; }
};

}

#endif // BARZER_BZSPELL_H 
