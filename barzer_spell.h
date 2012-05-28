#ifndef BARZER_SPELL_H
#define BARZER_SPELL_H
#include <map>
#include <barzer_dtaindex.h>
#include <ay_char.h>
#include <boost/thread/mutex.hpp>

class Hunspell;
class GlobalPools;
class StoredUniverse;
namespace barzer {

class BarzerHunspell {
	Hunspell* d_hunspell;
	StoredUniverse& d_universe;	
	mutable boost::mutex d_mutex;
public:
	boost::mutex& mutex() const { return d_mutex; }

	// this is a hack NEVER call this function
	BarzerHunspell( StoredUniverse& universe ) : d_hunspell(0), d_universe(universe) {}
	BarzerHunspell( StoredUniverse& universe, const char* affFile, const char* dictFile );
	~BarzerHunspell( );
	void initHunspell( const char* affFile, const char* dictFile );
	const StoredUniverse& getUniverse() const { return d_universe; }
	int addDictionary( const char* fname );
	int addWord( const char* );
	/// treats every line in the file as a word , adds it to dictionary  
	int addWordsFromTextFile( const char* fname );
	Hunspell* getHunspell() { return d_hunspell; }
	const Hunspell* getHunspell() const { return d_hunspell; }
};

/// data for a single invokation of the external spellchecker 
/// it can be used multiple times. the object cleans up memory allocated by 
/// the spellchecker
class BarzerHunspellInvoke {
	const BarzerHunspell& d_spell;
	
	typedef char* char_p;
	typedef char_p* char_pp;
	
	char_pp d_str_pp;
	size_t  d_str_pp_sz;

	/// levenshteyn edit distance calculator object - need an object because this thing 
	/// allocates memory 
	mutable ay::LevenshteinEditDistance d_editDist;

	Hunspell* getHunspell() { return const_cast<Hunspell*>( d_spell.getHunspell() ); }
	const GlobalPools& d_gp;
	bool d_isascii;
	/// results of ghettoXXX methods are stored in this variable
	mutable std::string d_ghettoStr;
public:
	/// if returns true d_ghettoStr will have the stem
	bool ghettoAsciiStem( const char* s ) const;
	ay::LevenshteinEditDistance& getEditDistanceCalc() const { return d_editDist; }

	void clear() ;
	BarzerHunspellInvoke( const BarzerHunspell& spell, const GlobalPools& gp  ) : 
		d_spell(spell) ,
		d_str_pp(0),
		d_str_pp_sz(0),
		d_gp(gp),
		d_isascii(true)
	{}

	~BarzerHunspellInvoke() { clear(); }

	/// returns number of spelling suggestions

	size_t getNumSpellSuggestions( const char* ) const { return d_str_pp_sz; }
	const char* getSpellSuggestion( size_t i ) const { return( i< d_str_pp_sz ? d_str_pp[i] : 0 ); }
	const char_p* getAllSuggestions() const { return d_str_pp; }
	
	/// return.first - ret value from Hunspell::spell() - 0 means it's a misspelling
	/// return.second is the number of suggestions
	std::pair< int, size_t> checkSpell( const char* s );

	const char* stem( const char* s );
};

} // barzer namespace ends
#endif // BARZER_SPELL_H
