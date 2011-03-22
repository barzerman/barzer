#ifndef BARZER_SUBCLASS_SUBCLASS_H
#define BARZER_SUBCLASS_SUBCLASS_H
namespace barzer {

//// constants 
namespace ling {

enum {
	WORD_SUBCLASS_NORMAL,
	WORD_SUBCLASS_IDSTYLE,     // stuff like BOEING737  or RXV550
	WORD_SUBCLASS_FLUFF,       // hey, hello 
	WORD_SUBCLASS_PRONOUN,     // i,we,you
	WORD_SUBCLASS_ARTICLE,     // a the 
	WORD_SUBCLASS_CONJUNCTION, // and or  
	WORD_SUBCLASS_PREPOSITION, // on to from ... 
	WORD_SUBCLASS_NEGATION     // not, nonm un-,nor
};

enum {
	WORD_FLAG_PLURAL
};

enum {
	NUM_SUBCLASS_DIGITS,
	NUM_SUBCLASS_TEXT       // ten thousand
};
enum {
	NUM_FLAG_ORDINAL,
	NUM_FLAG_CARDINAL,
	NUM_FLAG_NEGATIVE,
	NUM_FLAG_REAL, // 1.1231231
	NUM_FLAG_FRACTIONAL, // 1/2 or one half
	NUM_FLAG_POPCONST // popular constant - plank constant, e, pi
};

} // ling namespace
} // barzer namespace 
#endif // BARZER_FLAG_SUBCLASS_H
