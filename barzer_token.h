#ifndef BARZER_TOKEN_H
#define BARZER_TOKEN_H
#include <ay/ay_headers.h>
#include <ay/ay_bitflags.h>
/// Various low level constants and utilities 
/// used for both Parsed and Stored tokens
namespace barzer {

namespace tokutil {

} // tokutil namespace 

/// stored token info
/// this is filled out on load for each token known to DataIndex 
/// 
struct StoredTokenClassInfo {
	enum {
		CLASS_WORD,
		CLASS_NUMBER 
	};

	int16_t theClass;  // one of the CLASS_ values
	int16_t subclass;  // subclass within a given class values are in 
					       // barzer_parse_type_subclass.h in enums 
	enum { CLASS_SPEC_FLAG_MAX = 16 };
	ay::bitflags<CLASS_SPEC_FLAG_MAX>  classFlags; // class specific flags  also see subclass.h
	
	enum {
        /// word flags 
		BIT_GENERALDICT, // not present in the domain tokens, general english word
		BIT_COMPOUNDED, // thisis a compounded word
		BIT_MIXEDCASE, // token is in mixed case
		BIT_MISSPELLING, // this is a known misspelling
		BIT_STEM, // this is a  stem (doesn't exist as unstemmed)
        BIT_NUMBER, // this is a number
        BIT_HAS_UPPERCASE, // has upper case characters

		BIT_MAX
	};
	/// non-mutually exclusive binary properties
	ay::bitflags<BIT_MAX> flags; // bit mask of BIT_XXX
	
	StoredTokenClassInfo():
		theClass(CLASS_WORD),
		subclass(0)
	{}

	
	bool hasUpperCase() const { return flags[ BIT_HAS_UPPERCASE ]; }
    void setHasUpperCase() { flags.set( BIT_HAS_UPPERCASE, true ); }

	bool isSimple() const { return !flags[ BIT_COMPOUNDED ]; }
	void setSimple() { flags.set( BIT_COMPOUNDED, false ); }

	bool isCompounded() const { return flags[ BIT_COMPOUNDED ]; }
	void setCompounded() { flags.set( BIT_COMPOUNDED ); }

	bool isNumber() const { return flags[ BIT_NUMBER ]; }
	void setNumber() { flags.set( BIT_NUMBER ); }

	void setStemmed( bool mode ) { 
        if( mode ) 
            flags.set(BIT_STEM); 
        else 
            flags.unset(BIT_STEM);
    }
    bool isStemmed() const { return flags[BIT_STEM]; }


	void printClassName( std::ostream& ) const;
	void printSublassName( std::ostream& ) const;
	void printBitflagNames( std::ostream& ) const;
	void printClassflagNames( std::ostream& ) const;
	std::ostream& print( std::ostream& ) const;
};

////// parsing token class info 
//// this is filled by the lexer during parsing - highly volatile structure
struct CTokenClassInfo {
	typedef enum {
		CLASS_UNCLASSIFIED,
		CLASS_BLANK,
		CLASS_WORD,
		CLASS_MYSTERY_WORD, /// word cant be either classified or matched
		CLASS_NUMBER,
		CLASS_PUNCTUATION, // subClass/classFlags ill be always 0
		CLASS_SPACE, // subClass/classFlags ill be always 0 
	

		/// add new ones above this . 
		/// remember to update barzer_token.cpp (CTCI_ClassName)
		CLASS_MAX 
	} TokenClass_t;

	int16_t theClass;
	// subclass within a given class values are in 
   	// barzer_parse_type_subclass.h in enums 
	// for WORD and NUMBER classes subclasses should be the same
	// as the ones in StoredTokenClassInfo::subClass 
	int16_t subclass;  
public:
	enum { CLASS_SPEC_FLAG_MAX = 16 };
	ay::bitflags<CLASS_SPEC_FLAG_MAX>  classFlags; // class specific flags  also see subclass.h

	////// bits 
	enum {
		/// general bits
		CTCI_BIT_IN_DATASET, // word is matched and is in the dataset not just a
					        // general english word
		CTCI_BIT_COMPOUNDED,
		CTCI_BIT_MIXEDCASE, // token was not all lowercase
		CTCI_BIT_ORIGMATCH, // token matched as is without converting to lower case
		CTCI_BIT_SPELL_CORRECTED, // token was matched in the dictionary 
					       // only after spelling correction
		CTCI_BIT_STEMMED, // was only matched after being stemmed

		/// spelling bits 
		CTCI_BIT_SPELL_ALTERED, // simple word alteration (no split or join)
		CTCI_BIT_SPELL_SPLIT, // token was split
		CTCI_BIT_SPELL_JOINED, // tokens were joined

		/// all new bits must be added above this line 
		CTCI_BIT_MAX
	};
	
	ay::bitflags<CTCI_BIT_MAX> bitFlags;

	void setSpellCorrected( bool v = true ) { bitFlags.set( CTCI_BIT_SPELL_CORRECTED, true ); }
	void setStemmed( bool v = true ) { bitFlags.set( CTCI_BIT_STEMMED, true ); }

	bool  isSpellCorrected( ) const { return bitFlags[ CTCI_BIT_SPELL_CORRECTED ]; }
	bool  isStemmed( ) const { return bitFlags[CTCI_BIT_STEMMED]; }
	
	CTokenClassInfo( ) :
		theClass(CLASS_UNCLASSIFIED),
		subclass(0)
	{}
	void printClassSubclass( std::ostream& ) const;
	void printClassFlags( std::ostream& ) const;
	void printBitFlags( std::ostream& ) const;

	std::ostream& print( std::ostream& ) const;
	void clear() {
		theClass = CLASS_BLANK;
		subclass = 0;
		bitFlags.clear();
		classFlags.clear();
	}
};

inline std::ostream& operator <<( std::ostream& fp, const CTokenClassInfo& t )
{ return t.print(fp); }

// linugistic information - part of speech, directionality etc
struct TokenLinguisticInfo {
	/// unused
	uint8_t dummy;
	void clear() {}
	std::ostream& print( std::ostream& fp ) const
	{ return fp; }
};
inline std::ostream& operator <<( std::ostream& fp, const TokenLinguisticInfo& t)
{ return t.print(fp); }

} // barzer namespace ends
#endif // BARZER_TOKEN_H
