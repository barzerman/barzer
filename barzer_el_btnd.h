#ifndef BARZER_EL_BTND_H
#define BARZER_EL_BTND_H
// types defined in this file are used to store information for various node types 
// in the BarzEL trie
namespace barzer {

/// simple number wildcard data 
struct BTND_Number {
	enum {
		T_ANY_INT, // any integer 
		T_ANY_REAL, // any real

		T_EXACT_INT, // specific integer 
		T_EXACT_REAL,  // exact real with epsilon 0.0000000001
		
		T_RANGE_INT, // range is always inclusive as in <= <=
		T_RANGE_REAL, // same as above - always inclusive

		T_MAX
	};
	uint8_t type; // one of T_XXXX 
	// for EXACT_XXX types only lo is used
	union {
		struct { float lo,hi; } real;
		struct { int 	lo,hi; } integer;
	} range;
};

// 
struct BTND_Punct {
	int theChar; // actual punctuation character
};
// simple token wildcard data
struct BTND_Wildcard {
	uint8_t minTerms, maxTerms;
};

// date wildcard data
struct BTND_Date {
};

// time wildcard data
struct BTND_Time {
};

struct BTND_Token {
};

}

#endif // BARZER_EL_BTND_H
