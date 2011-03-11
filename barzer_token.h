#ifndef BARZER_TOKEN_H
#define BARZER_TOKEN_H
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
		BIT_GENERALDICT, // not present in the domain tokens, general english word
		BIT_COMPOUNDED,
		BIT_MIXEDCASE, // token is in mixed case
		BIT_MISSPELLING, // this is a known misspelling
		BIT_STEM, // this is a  stem (doesn't exist as unstemmed)

		BIT_MAX
	};
	/// non-mutually exclusive binary properties
	ay::bitflags<BIT_MAX> flags; // bit mask of BIT_XXX
	
	StoredTokenClassInfo():
		theClass(CLASS_WORD),
		subclass(0)
	{}
};

////// parsing token class info 
//// this is filled by the lexer during parsing - highly volatile structure
struct CTokenClassInfo {
	typedef enum {
		CLASS_UNCLASSIFIED,
		CLASS_WORD,
		CLASS_MYSTERY_WORD, /// word cant be either classified or matched
		CLASS_NUMBER,
		CLASS_PUNCTUATION, // subClass/classFlags ill be always 0
		CLASS_SPACE // subClass/classFlags ill be always 0 
	} TokenClass_t;

	int16_t theClass;
	int16_t subclass;  // subclass within a given class values are in 
					       // barzer_parse_type_subclass.h in enums 
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
	
	CTokenClassInfo( ) :
		theClass(CLASS_UNCLASSIFIED),
		subclass(0)
	{}
};

// linugistic information - part of speech, directionality etc
class TokenLinguisticInfo {
	/// unused
	uint8_t dummy;
};

} // barzer namespace ends
#endif // BARZER_TOKEN_H
