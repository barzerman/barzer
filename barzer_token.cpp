#include <barzer_token.h>
#include <barzer_type_subclass.h>
#include <barzer_storage_types.h>
#include <ay_util.h>

namespace barzer {
/// STCI - stands for StoredTokenClassInfo
struct EnumAndString {
	int num;
	const char* s;
	
	/// locates theEnum and prints name
	static void printSingleName( 
		std::ostream& fp, 
		const EnumAndString* buf,
		size_t buf_sz,
		int theEnum
	)
	{
		for( size_t i =0; i< buf_sz; ++i ) {
			const EnumAndString& eas = buf[i];
			if( eas.num == theEnum ) {
				fp << eas.s ;
				return;
			}
		}
		fp << "(undefined)";
	}
};

///// Stored Token methods and constants 
///// class names
#define STCI_CLASS_NAME( x ) { StoredTokenClassInfo::CLASS_##x, #x }
static EnumAndString g_STCI_ClassName[] = {
	STCI_CLASS_NAME( WORD ),
	STCI_CLASS_NAME( NUMBER )
};
#undef STCI_CLASS_NAME

/// subclass names

/// WORD sublasses
#define STCI_WORD_SUBCLASS_NAME( x ) { ling::WORD_SUBCLASS_##x, #x }
static EnumAndString g_STCI_SubclassName_Word[] = {
	STCI_WORD_SUBCLASS_NAME(NORMAL),
	STCI_WORD_SUBCLASS_NAME(IDSTYLE),     // stuff like BOEING737  or RXV550
	STCI_WORD_SUBCLASS_NAME(FLUFF),       // hey, hello 
	STCI_WORD_SUBCLASS_NAME(PRONOUN),     // i,we,you
	STCI_WORD_SUBCLASS_NAME(ARTICLE),     // a the 
	STCI_WORD_SUBCLASS_NAME(CONJUNCTION), // and or  
	STCI_WORD_SUBCLASS_NAME(PREPOSITION), // on to from ... 
	STCI_WORD_SUBCLASS_NAME(NEGATION)     // not), nonm un-,nor
};
#undef STCI_WORD_SUBCLASS_NAME

/// WORD flags
#define STCI_WORD_CLASSFLAG_NAME( x ) { ling::WORD_FLAG_##x, #x }
static EnumAndString g_STCI_ClassFlag_Word[] =  {
	STCI_WORD_CLASSFLAG_NAME(PLURAL)
};
#undef STCI_WORD_CLASSFLAG_NAME

#define STCI_NUMBER_SUBCLASS_NAME( x ) { ling::NUM_SUBCLASS_##x, #x }
static EnumAndString g_STCI_SubclassName_Number[] = {
	STCI_NUMBER_SUBCLASS_NAME(DIGITS),
	STCI_NUMBER_SUBCLASS_NAME(TEXT)       // ten thousand
};
#undef STCI_NUMBER_SUBCLASS_NAME

#define STCI_NUMBER_CLASSFLAG_NAME( x ) { ling::NUM_FLAG_##x, #x }
static EnumAndString g_STCI_ClassFlag_Number[] =  {
	STCI_NUMBER_CLASSFLAG_NAME(ORDINAL),
	STCI_NUMBER_CLASSFLAG_NAME(CARDINAL),
	STCI_NUMBER_CLASSFLAG_NAME(NEGATIVE),
	STCI_NUMBER_CLASSFLAG_NAME(REAL),
	STCI_NUMBER_CLASSFLAG_NAME(FRACTIONAL),
	STCI_NUMBER_CLASSFLAG_NAME(POPCONST)
};
#undef STCI_NUMBER_CLASSFLAG_NAME

#define STCI_BIT_NAME( x ) { StoredTokenClassInfo::BIT_##x, #x }
static EnumAndString g_STCI_BitflagName[] = {
		STCI_BIT_NAME(GENERALDICT),
		STCI_BIT_NAME(COMPOUNDED),
		STCI_BIT_NAME(MIXEDCASE),
		STCI_BIT_NAME(MISSPELLING),
		STCI_BIT_NAME(STEM)
};
#undef STCI_BIT_NAME


/*
void EnumAndString::printSingleName( 
		std::ostream& fp, 
		const EnumAndString* buf,
		size_t buf_sz,
		int theEnum
	);
*/

/*
static EnumAndString g_STCI_ClassName
static EnumAndString g_STCI_SubclassName_Word
static EnumAndString g_STCI_SubclassName_Number
static EnumAndString g_STCI_ClassFlag_Number
static EnumAndString g_STCI_ClassFlag_Word
static EnumAndString g_STCI_BitflagName
*/

void StoredTokenClassInfo::printClassName( std::ostream& fp ) const
{
	EnumAndString::printSingleName( fp, g_STCI_ClassName, ARR_SZ(g_STCI_ClassName), theClass );
}
void StoredTokenClassInfo::printSublassName( std::ostream& fp ) const
{
	switch( theClass ) {
	case CLASS_WORD:
		EnumAndString::printSingleName( fp, g_STCI_SubclassName_Word, ARR_SZ(g_STCI_SubclassName_Word), subclass );
		break;
	case CLASS_NUMBER:
		EnumAndString::printSingleName( fp, g_STCI_SubclassName_Number, ARR_SZ(g_STCI_SubclassName_Number), subclass );
		break;
	default:
		fp << "(undefined)";
		break;
	}
}
void StoredTokenClassInfo::printBitflagNames( std::ostream& fp ) const
{
	bool hasBits = false;
	for( uint8_t i = BIT_GENERALDICT; i< BIT_MAX; ++i ) {
		if( flags[i] ) {
			if( hasBits ) 
				fp << '|';
			else
				hasBits = true;
			/// this is slower but safer 
			EnumAndString::printSingleName( 
				fp, g_STCI_BitflagName, ARR_SZ(g_STCI_BitflagName), i 
			);
		}
	}
}
void StoredTokenClassInfo::printClassflagNames( std::ostream& fp ) const
{
	uint8_t maxBit = 0;

	const EnumAndString *eas= 0;
	//g_STCI_ClassFlag_Number
	//g_STCI_ClassFlag_Word
	switch( theClass ) {
	case CLASS_WORD:
		eas = g_STCI_ClassFlag_Word;
		maxBit = (uint8_t)(ARR_SZ(g_STCI_ClassFlag_Word));
		break;
	case CLASS_NUMBER:
		eas = g_STCI_ClassFlag_Number;
		maxBit = (uint8_t)(ARR_SZ(g_STCI_ClassFlag_Number));
		break;
	default:
		fp<< "(undefined)";
		return;
	}
	bool hasBits = false;
	for( uint8_t i = 0; i< maxBit; ++i ) {
		if( classFlags[i] ) {
			if( hasBits ) 
				fp << '|';
			else
				hasBits = true;
			/// slower but safer
			EnumAndString::printSingleName( 
				fp, eas, maxBit, i 
			);
		}
	}
}

/*
void printClassName( std::ostream& ) const;
void printSublassName( std::ostream& ) const;
void printBitflagNames( std::ostream& ) const;
void printClassflagNames( std::ostream& ) const;
*/

std::ostream& StoredTokenClassInfo::print( std::ostream& fp ) const
{
	
	printClassName( fp ) ;
	fp << ',';
	printSublassName(fp);
	fp << "[";
	printBitflagNames(fp) ;
	fp << "]:[";
	printClassflagNames(fp) ;
	fp << "]";
	return fp;
}

/////// end of stored token methods
/////// CToken methods
/// CTCI - CTokenClassInfo
#define CTCI_CLASS_NAME( x ) { CTokenClassInfo::CLASS_##x, #x }
static EnumAndString g_CTCI_ClassName[] = {
		CTCI_CLASS_NAME(UNCLASSIFIED),
		CTCI_CLASS_NAME(WORD),
		CTCI_CLASS_NAME(MYSTERY_WORD),
		CTCI_CLASS_NAME(NUMBER),
		CTCI_CLASS_NAME(PUNCTUATION),
		CTCI_CLASS_NAME(SPACE)
};
#undef CTCI_NUMBER_SUBCLASS_NAME
#define CTCI_BITFLAG_NAME( x ) { CTokenClassInfo::CTCI_BIT_##x, #x }
static EnumAndString g_CTCI_BitflagName[] = {
		CTCI_BITFLAG_NAME(IN_DATASET),
		CTCI_BITFLAG_NAME(COMPOUNDED),
		CTCI_BITFLAG_NAME(MIXEDCASE),
		CTCI_BITFLAG_NAME(ORIGMATCH),
		CTCI_BITFLAG_NAME(SPELL_CORRECTED),
		CTCI_BITFLAG_NAME(STEMMED),
		CTCI_BITFLAG_NAME(SPELL_ALTERED),
		CTCI_BITFLAG_NAME(SPELL_SPLIT),
		CTCI_BITFLAG_NAME(SPELL_JOINED)
};
#undef CTCI_BITFLAG_NAME

void CTokenClassInfo::printClassSubclass( std::ostream& fp ) const
{
	EnumAndString::printSingleName( fp, g_CTCI_ClassName, ARR_SZ(g_CTCI_ClassName), theClass );
	fp << ",";
	switch( theClass ) {
	case CLASS_WORD:
		EnumAndString::printSingleName( fp, g_STCI_SubclassName_Word, ARR_SZ(g_STCI_SubclassName_Word), subclass );
		break;
	case CLASS_NUMBER:
		EnumAndString::printSingleName( fp, g_STCI_SubclassName_Number, ARR_SZ(g_STCI_SubclassName_Number), subclass );
		break;
	case CLASS_UNCLASSIFIED:
	case CLASS_MYSTERY_WORD:
	case CLASS_PUNCTUATION:
	case CLASS_SPACE:
		fp << "_";
		break;
	default:
		fp << "(unknown)";
		break;
	}
}
void CTokenClassInfo::printClassFlags( std::ostream& fp ) const
{
	// Right now CToken class flags are either the same as the 
	// StoredToken class flags or inapplicable 
}
void CTokenClassInfo::printBitFlags( std::ostream& fp ) const
{
	bool hasBits = false;
	for( uint8_t i = 0; i< CTCI_BIT_MAX; ++i ) {
		if( bitFlags[i] ) {
			if( hasBits ) 
				fp << '|';
			else
				hasBits = true;
			/// this is slower but safer 
			EnumAndString::printSingleName( 
				fp, g_CTCI_BitflagName, ARR_SZ(g_CTCI_BitflagName), i 
			);
		}
	}
}
std::ostream& CTokenClassInfo::print( std::ostream& fp ) const
{
	printClassSubclass( fp );
	fp << ":";
	printClassFlags( fp );
	fp << "{";
	printBitFlags( fp );
	fp << "}";
	return fp;
}

} // barzer namespace 
