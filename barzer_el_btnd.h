#ifndef BARZER_EL_BTND_H
#define BARZER_EL_BTND_H

#include <ay/ay_bitflags.h>
#include <ay/ay_string_pool.h>
#include <boost/variant.hpp>

// types defined in this file are used to store information for various node types 
// in the BarzEL trie
namespace barzer {

//// BarzelTrieNodeData - PATTERN types 

/// simple number wildcard data 
struct BTND_Pattern_Number {
	enum {
		T_ANY_INT, // any integer 
		T_ANY_REAL, // any real

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

	bool isReal() const 
		{ return (type == T_ANY_REAL || type == T_RANGE_REAL); }

	BTND_Pattern_Number() : 
		type(T_ANY_INT)
	{ range.integer.lo = range.integer.hi = 0; }

	void setAnyInt() { type = T_ANY_INT; }
	void setAnyReal() { type = T_ANY_REAL; }

	void setIntRange( int lo, int hi )
	{
		type = T_RANGE_INT;
		range.integer.lo = lo;
		range.integer.hi = hi;
	}
	void setRealRange( float lo, float hi )
	{
		type = T_RANGE_REAL;
		range.real.lo = lo;
		range.real.hi = hi;
	}
};

// 
struct BTND_Pattern_Punct {
	int theChar; // actual punctuation character
	BTND_Pattern_Punct() : theChar(0) {}
	BTND_Pattern_Punct(char c) : theChar(c) {}
};
// simple token wildcard data
struct BTND_Pattern_Wildcard {
	uint8_t minTerms, maxTerms;
	BTND_Pattern_Wildcard() : minTerms(0), maxTerms(0) {}
	BTND_Pattern_Wildcard(uint8_t mn, uint8_t mx ) : minTerms(mn), maxTerms(mx) {}
};

// date wildcard data
struct BTND_Pattern_Date {
	enum {
		T_ANY_DATE, 
		T_ANY_FUTUREDATE, 
		T_ANY_PASTDATE, 

		T_DATERANGE, // range of dates
		T_MAX
	};
	uint8_t type; // T_XXXX values 
	uint32_t lo, hi; // low high date in YYYYMMDD format
	
	BTND_Pattern_Date( ) : type(T_ANY_DATE), lo(0),hi(0) {}
};

// time wildcard data. represents time of day 
// HHMM - short number
struct BTND_Pattern_Time {
	enum {
		T_ANY_TIME,
		T_TIMERANGE
	};
	uint8_t type; // T_XXXX values 
	uint16_t lo, hi; // HHMM 

	BTND_Pattern_Time( ) : type(T_ANY_TIME), lo(0), hi(0) {}
};

struct BTND_Pattern_DateTime {
	enum {
		T_ANY_DATETIME, 
		T_DATETIME_RANGE,
		T_MAX
	};
	
	uint8_t type; // T_XXXX values 
	uint16_t tlo,thi; // HHMM lo hi time
	uint16_t dlo,dhi; // YYYYMMDD lo hi date

	BTND_Pattern_DateTime() : type(T_ANY_DATETIME) , 
		tlo(0), thi(0), dlo(0), dhi(0) {}
};

struct BTND_Pattern_CompoundedWord {
	uint32_t compWordId;  
	BTND_Pattern_CompoundedWord() :
		compWordId(0xffffffff)
	{}
	BTND_Pattern_CompoundedWord(uint32_t cwi ) : compWordId(cwi) {}
};

struct BTND_Pattern_Token {
	ay::UniqueCharPool::StrId stringId;

	BTND_Pattern_Token() : stringId(ay::UniqueCharPool::ID_NOTFOUND) {}
	BTND_Pattern_Token(ay::UniqueCharPool::StrId id) : stringId(id) {}
};

/// blank data type 
struct BTND_Pattern_None {
};

typedef boost::variant< 
		BTND_Pattern_None,				// 0
		BTND_Pattern_Token, 			// 1
		BTND_Pattern_Punct,  			// 2
		BTND_Pattern_CompoundedWord, 	// 3
		BTND_Pattern_Number, 			// 4
		BTND_Pattern_Wildcard, 			// 5
		BTND_Pattern_Date,     			// 6
		BTND_Pattern_Time,     			// 7
		BTND_Pattern_DateTime 			// 8 
> BTND_PatternData;

/// these enums must mirror the order of the types in BTND_PatternData
enum {
	BTND_Pattern_None_TYPE, 			// 0			
	BTND_Pattern_Token_TYPE, 			// 1			
	BTND_Pattern_Punct_TYPE,  		    // 2
	BTND_Pattern_CompoundedWord_TYPE,	// 3
	BTND_Pattern_Number_TYPE, 			// 4
	BTND_Pattern_Wildcard_TYPE, 		// 5
	BTND_Pattern_Date_TYPE,     		// 6
	BTND_Pattern_Time_TYPE,    			// 7
	BTND_Pattern_DateTime_TYPE,			// 8
	
	BTND_Pattern_TYPE_MAX
};


//// REWRITE Types
struct BTND_Rewrite_Literal {
	enum {
		T_STRING,
		T_COMPOUND
	};
	uint8_t  type;
	uint32_t theId;

	BTND_Rewrite_Literal() : 
		type(T_STRING),
		theId(ay::UniqueCharPool::ID_NOTFOUND) 
	{}
	void setCompound(uint32_t id ) 
		{ type = T_COMPOUND; theId = id; }

	void setCompound()
		{ setCompound(ay::UniqueCharPool::ID_NOTFOUND) ; }
	void setString(uint32_t id) 
		{ type = T_STRING; theId = id;  }
};

struct BTND_Rewrite_Number {
	bool isConst; /// when false we will need to evaluate underlying nodes
	boost::variant< int, double > val;
	BTND_Rewrite_Number( ) : isConst(false) {}

	BTND_Rewrite_Number( int x ) : isConst(true), val(x) {}
	BTND_Rewrite_Number( double x ) : isConst(true), val(x) {}

	void set( int i ) { isConst = true; val =i; }
	void set( double i ) { isConst = true; val =i; }
};

struct BTND_Rewrite_Variable {
	bool byName; 
	uint32_t varId; /// when byName is true this will be used as a sring id
		// otherwise as $1/$2 
};

struct BTND_Rewrite_Function {
	ay::UniqueCharPool::StrId nameId; // function name id
	BTND_Rewrite_Function() : nameId(ay::UniqueCharPool::ID_NOTFOUND) {}
	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id) : nameId(id) {}
};
// blank rewrite data type
struct BTND_Rewrite_None {
	int dummy;
	BTND_Rewrite_None() : dummy(0) {}
};

typedef boost::variant< 
	BTND_Rewrite_None,
	BTND_Rewrite_Literal,
	BTND_Rewrite_Number,
	BTND_Rewrite_Variable,
	BTND_Rewrite_Function
> BTND_RewriteData;

enum {
	BTND_Rewrite_None_TYPE,
	BTND_Rewrite_Literal_TYPE,
	BTND_Rewrite_Number_TYPE,
	BTND_Rewrite_Variable_TYPE,
	BTND_Rewrite_Function_TYPE
}; 

/// when updating the enums make sure to sunc up BTNDDecode::typeName_XXX 

/// pattern structure flow data - blank for now
struct BTND_StructData {
	enum {
		T_LIST, // sequence of elements
		T_ANY,  // any element 
		T_OPT,  // optional subtree
		T_PERM, // permutation of children
		T_TAIL, // if children are A,B,C this translates into [A], [A,B] and [A,B,C]
		/// add new types above this line only
		BEL_STRUCT_MAX
	};
	int type;
	BTND_StructData() : type(T_LIST) {}
	BTND_StructData(int t) : type(t) {}
};

/// blank data type
struct BTND_None {
};

typedef boost::variant<
	BTND_None, 		  // 0
	BTND_StructData,  // 1
	BTND_PatternData, // 2
	BTND_RewriteData  // 3
> BTNDVariant;

enum {
	BTND_None_TYPE, // 0
	BTND_StructData_TYPE,  // 1
	BTND_PatternData_TYPE, // 2
	BTND_RewriteData_TYPE  // 3
};

struct BTNDDecode {
	static const char* typeName_Pattern ( int x ) ; // BTND_Pattern_XXXX_TYPE values 
	static const char* typeName_Rewrite ( int x ) ; // BTND_Rewrite_XXXX_TYPE values 
	static const char* typeName_Struct ( int x ) ; // BTND_Struct_XXXX_TYPE values 
};

} // barzer namespace

#endif // BARZER_EL_BTND_H
