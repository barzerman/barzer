#ifndef BARZER_SPELL_H
#define BARZER_SPELL_H
#include <map>
#include <barzer_dtaindex.h>

class Hunspell;
namespace barzer {

class BarzerHunspell {
	Hunspell* d_hunspell;
public:
	BarzerHunspell( const char* affFile, const char* dictFile );
	~BarzerHunspell( );
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
	BarzerHunspell& d_spell;
	
	typedef char* char_p;
	typedef char_p* char_pp;
	
	char_pp d_str_pp;
	size_t  d_str_pp_sz;
public:
	void clear() ;
	BarzerHunspellInvoke( BarzerHunspell& spell ) : 
		d_spell(spell) ,
		d_str_pp(0),
		d_str_pp_sz(0)
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

/// barzer spell has a few lines of defence 
/// first it tries to match the string that couldnt be matched using the prefix (maybe sugfix also) tries
/// only very certain matches will stop at that point 
/// if no conclusive match could be done using the tries it invokes external speller 
/// 

class BarzerSpell  {
	BarzerHunspell& d_extSpell;
	
	DtaIndex& d_dtaIdx;
public:
	BarzerSpell( BarzerHunspell& espell, DtaIndex& dtaIdx ) : d_extSpell(espell), d_dtaIdx(dtaIdx)  {}

};

} // barzer namespace ends
#endif // BARZER_SPELL_H
