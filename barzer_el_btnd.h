#ifndef BARZER_EL_BTND_H
#define BARZER_EL_BTND_H

#include <stdint.h>
#include <ay/ay_bitflags.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util.h>
#include <boost/variant.hpp>
#include <barzer_basic_types.h>

// types defined in this file are used to store information for various node types 
// in the BarzEL trie
namespace barzer {
struct BELPrintContext;

//// BarzelTrieNodeData - PATTERN types 

/// simple number wildcard data 
struct BTND_Pattern_Number {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
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
		struct { int lo,hi; } integer;
	} range;

	inline bool isLessThan( const BTND_Pattern_Number& r ) const
	{
		if( type < r.type ) 
			return true;
		else if( r.type < type ) 
			return false;
		else {
			switch( type ) {
			case T_ANY_INT: return false;
			case T_ANY_REAL: return false;
			case T_RANGE_INT: 
				return (ay::range_comp().less_than(
						range.integer.lo, range.integer.hi,
						r.range.integer.lo, r.range.integer.hi
					));
			case T_RANGE_REAL: 
				return ay::range_comp().less_than(
						range.real.lo, range.real.hi,
						r.range.real.lo, r.range.real.hi
					);
			default:
				return false;
			}
		}
	}

	bool isReal() const 
		{ return (type == T_ANY_REAL || type == T_RANGE_REAL); }

	BTND_Pattern_Number() : 
		type(T_ANY_INT)
	{ range.integer.lo = range.integer.hi = 0; }

	void setAnyInt() { type = T_ANY_INT; }
	void setAnyReal() { type = T_ANY_REAL; }

	int getType() const { return type; }
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
	std::ostream& printRange( std::ostream& fp ) const 
	{
		switch( type ) {
		case T_RANGE_REAL:
			return (fp << range.real.lo << "," << range.real.hi);
		case T_RANGE_INT:
			return (fp << range.integer.lo << "," << range.integer.hi);
		}
		return fp;
	}
	bool checkNumber( const BarzerNumber& num ) const;
};
inline bool operator <( const BTND_Pattern_Number& l, const BTND_Pattern_Number& r )
	{ return l.isLessThan( r ); }
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Number& x )
{
	return (x.printRange( fp << "NUM[" ) << "]");
}

// Punctuation and Stop Tokens (theChar is 0 for stops) 
struct BTND_Pattern_Punct {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	int theChar; // actual punctuation character. 0 - same as stop
	
	BTND_Pattern_Punct() : theChar(0) {}
	BTND_Pattern_Punct(char c) : theChar(c) {}

	void setChar( char c ) { theChar = c; }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Punct& x )
	{ return( fp << "'" << std::hex << x.theChar << "'" ); }

// simple token wildcard data
struct BTND_Pattern_Wildcard {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint8_t minTerms, maxTerms;

	size_t getMinTokSpan() const { return minTerms; }
	size_t getMaxTokSpan() const { return maxTerms; }

	BTND_Pattern_Wildcard() : minTerms(0), maxTerms(0) {}
	BTND_Pattern_Wildcard(uint8_t mn, uint8_t mx ) : minTerms(mn), maxTerms(mx) {}

	bool isLessThan( const BTND_Pattern_Wildcard& r ) const
	{
		return( ay::range_comp().less_than( minTerms, maxTerms, r.minTerms, r.maxTerms ) );
	}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Wildcard& x )
	{ return( fp << "*[" << x.minTerms << "," << x.maxTerms << "]" ); }
inline bool operator <( const BTND_Pattern_Wildcard& l, const BTND_Pattern_Wildcard& r )
	{ return l.isLessThan( r ); }

// date wildcard data
struct BTND_Pattern_Date {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
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

	bool isLessThan( const BTND_Pattern_Date& r ) const
		{ return ay::range_comp().less_than( type, lo, hi, r.type, r.lo, r.hi ); }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Date& x )
	{ return( fp << "date." << x.type << "[" << x.lo << "," << x.hi << "]" ); }

inline bool operator <( const BTND_Pattern_Date& l, const BTND_Pattern_Date& r )
	{ return l.isLessThan( r ); }

// time wildcard data. represents time of day 
// HHMM - short number
struct BTND_Pattern_Time {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_ANY_TIME,
		T_ANY_FUTURE_TIME,
		T_ANY_PAST_TIME,
		T_TIMERANGE
	};
	uint8_t type; // T_XXXX values 
	uint16_t lo, hi; // HHMM 

	BTND_Pattern_Time( ) : type(T_ANY_TIME), lo(0), hi(0) {}
	bool isLessThan( const BTND_Pattern_Time& r ) const
		{ return ay::range_comp().less_than( type,lo, hi, r.type, r.lo, r.hi ); }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Time& x )
	{ return( fp << "time." << x.type << "[" << x.lo << "," << x.hi << "]" ); }
inline bool operator <( const BTND_Pattern_Time& l, const BTND_Pattern_Time& r )
{
	return l.isLessThan( r );
}


struct BTND_Pattern_DateTime {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_ANY_DATETIME, 
		T_ANY_FUTURE_DATETIME, 
		T_ANY_PAST_DATETIME, 
		T_DATETIME_RANGE,
		T_MAX
	};
	
	uint8_t type; // T_XXXX values 
	uint16_t tlo,thi; // HHMM lo hi time
	uint32_t dlo,dhi; // YYYYMMDD lo hi date

	BTND_Pattern_DateTime() : type(T_ANY_DATETIME) , 
		tlo(0), thi(0), dlo(0), dhi(0) {}
	
	bool isLessThan( const BTND_Pattern_DateTime& r ) const
	{
		if( type < r.type  ) 
			return true;
		else if( r.type < type ) 
			return false;

		switch( type ) {
		case T_ANY_DATETIME:
			return false;
		case T_DATETIME_RANGE: 
			ay::range_comp().less_than( 
				dlo,   dhi,   tlo,   thi, 
				r.dlo, r.dhi, r.tlo, r.thi );
		default: return false;
		}
	}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_DateTime& x )
	{ return( fp << "datetime." << x.type << "[" << 
		x.dlo << ':' << x.tlo  << "," << 
		x.dhi << ':' << x.thi << "]" ); }

inline bool operator <( const BTND_Pattern_DateTime& l, const BTND_Pattern_DateTime& r )
	{ return l.isLessThan( r ); }

struct BTND_Pattern_CompoundedWord {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint32_t compWordId;  
	BTND_Pattern_CompoundedWord() :
		compWordId(0xffffffff)
	{}
	BTND_Pattern_CompoundedWord(uint32_t cwi ) : compWordId(cwi) {}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_CompoundedWord& x )
	{ return( fp << "compw[" << std::hex << x.compWordId << "]");}

struct BTND_Pattern_Token {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	ay::UniqueCharPool::StrId stringId;

	BTND_Pattern_Token() : 
		stringId(ay::UniqueCharPool::ID_NOTFOUND)
	{}
	BTND_Pattern_Token(ay::UniqueCharPool::StrId id) : 
		stringId(id)
	{}
};
/// stop token is a regular token literal except it will be ignored 
/// by any tag matching algorithm
struct BTND_Pattern_StopToken : public BTND_Pattern_Token {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
	{
		return BTND_Pattern_Token::print( fp, ctxt ) << "<STOP>";
	}
	ay::UniqueCharPool::StrId stringId;

	BTND_Pattern_StopToken(ay::UniqueCharPool::StrId id) : 
		BTND_Pattern_Token(id)
	{}
};

inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_StopToken& x )
	{ return( fp << "stop[" << std::hex << x.stringId << "]");}
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Token& x )
	{ return( fp << "string[" << std::hex << x.stringId << "]");}

/// blank data type 
struct BTND_Pattern_None {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_None& x )
	{ return( fp << "<none>" ); }

typedef boost::variant< 
		BTND_Pattern_None,				// 0
		BTND_Pattern_Token, 			// 1
		BTND_Pattern_Punct,  			// 2
		BTND_Pattern_CompoundedWord, 	// 3
		BTND_Pattern_Number, 			// 4
		BTND_Pattern_Wildcard, 			// 5
		BTND_Pattern_Date,     			// 6
		BTND_Pattern_Time,     			// 7
		BTND_Pattern_DateTime, 			// 8 
		BTND_Pattern_StopToken		 	// 9
> BTND_PatternData;


/// these enums MUST mirror the order of the types in BTND_PatternData
enum {
	BTND_Pattern_None_TYPE, 			// 0			
	BTND_Pattern_Token_TYPE, 			// 1			
	BTND_Pattern_Punct_TYPE,  		    // 2
	BTND_Pattern_CompoundedWord_TYPE,	// 3
	/// wildcard types 
	BTND_Pattern_Number_TYPE, 			// 4
	BTND_Pattern_Wildcard_TYPE, 		// 5
	BTND_Pattern_Date_TYPE,     		// 6
	BTND_Pattern_Time_TYPE,    			// 7
	BTND_Pattern_DateTime_TYPE,			// 8
	///
	BTND_Pattern_StopToken_TYPE,		// 9


	/// end of wildcard types - add new ones ONLY ABOVE THIS LINE
	/// in general there should be no more than 1-2 more types added 
	/// nest them in another variant 
	BTND_Pattern_MAXWILDCARD_TYPE,
	
	BTND_Pattern_TYPE_MAX
};

/// BTND pattern accessor - ghetto visitor 
struct BTND_PatternData_Access {
	bool isWildcard( const BTND_PatternData& btnd ) const 
	{
		return( btnd.which() >= BTND_Pattern_Number_TYPE && btnd.which() < BTND_Pattern_MAXWILDCARD_TYPE );
	}

	/// returns the maximum tok span for the wildcard 
	/// generally most things except for the 'Wildcard' (this one skips an arbitrary number of terms 
	/// have the span of 1  
	inline size_t getMaxTokSpan(const BTND_PatternData& btnd ) const 
	{
		if( btnd.which() == BTND_Pattern_Wildcard_TYPE ) 
			return boost::get< BTND_Pattern_Wildcard >(btnd).getMaxTokSpan();
		else 
			return 1;
	}
	inline size_t getMinTokSpan(const BTND_PatternData& btnd ) const 
	{
		if( btnd.which() == BTND_Pattern_Wildcard_TYPE ) 
			return boost::get< BTND_Pattern_Wildcard >(btnd).getMinTokSpan();
		else 
			return 1;
	}
};


typedef std::vector< BTND_PatternData > BTND_PatternDataVec;

//// REWRITE Types
struct BTND_Rewrite_Literal : public  BarzerLiteral {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
		{ return BarzerLiteral::print( fp, ctxt ); }
};

struct BTND_Rewrite_Number : public BarzerNumber {
	
	uint8_t isConst; /// when false we will need to evaluate underlying nodes

	BTND_Rewrite_Number( ) : isConst(0) {}

	BTND_Rewrite_Number( int x ) : BarzerNumber(x), isConst(0) {}
	BTND_Rewrite_Number( double x ) : BarzerNumber(x), isConst(0) {}

	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerNumber::print( fp ); }
	void set( int i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set( double i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
};

struct BTND_Rewrite_Variable {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		MODE_WC_NUMBER, // wildcard number
		MODE_VARNAME,   // variable name 
		MODE_PATEL_NUMBER // pattern element number
	};
	uint8_t idMode; 
	uint32_t varId; /// when idMode is MODE_VARNAME this will be used as a sring id
		// otherwise as $1/$2 

	bool isValid() const { return varId != 0xffffffff; }

	bool isVarname() const { return idMode == MODE_VARNAME; }
	bool isWildcardNum() const { return idMode == MODE_WC_NUMBER; }
	bool isPatternElemNumber() const { return idMode == MODE_PATEL_NUMBER; }

	void setVarNameId( uint32_t vid ){ varId = vid; idMode= MODE_VARNAME; }
	void setWildcardNumber( uint32_t vid ){ varId = vid; idMode= MODE_WC_NUMBER; }
	void setPatternElemNumber( uint32_t vid ){ varId = vid; idMode= MODE_PATEL_NUMBER; }

	BTND_Rewrite_Variable() : 
		idMode(MODE_WC_NUMBER), 
		varId(0xffffffff) 
	{}
};

struct BTND_Rewrite_Function {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	ay::UniqueCharPool::StrId nameId; // function name id
	BTND_Rewrite_Function() : nameId(ay::UniqueCharPool::ID_NOTFOUND) {}
	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id) : nameId(id) {}
};
// blank rewrite data type
struct BTND_Rewrite_None {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	int dummy;
	BTND_Rewrite_None() : dummy(0) {}
};
/// absolute date will always default to today
/// if it has child nodes it will be 
struct BTND_Rewrite_AbsoluteDate : public BarzerDate {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerDate::print( fp ); }
};

struct BTND_Rewrite_TimeOfDay : public BarzerTimeOfDay {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerTimeOfDay::print( fp ); }
};

enum {
	BTND_Rewrite_AbsoluteDate_TYPE,
	BTND_Rewrite_TimeOfDay_TYPE
};

/// they may actually store an explicit constant range
/// if that werent an issue we could make it a blank type
struct BTND_Rewrite_Range : public BarzerRange {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerRange::print( fp ); }
};

typedef boost::variant<
	BTND_Rewrite_AbsoluteDate,
	BTND_Rewrite_TimeOfDay
> BTND_Rewrite_DateTime;

typedef boost::variant< 
	BTND_Rewrite_None,
	BTND_Rewrite_Literal,
	BTND_Rewrite_Number,
	BTND_Rewrite_Variable,
	BTND_Rewrite_Function,
	BTND_Rewrite_DateTime,
	BTND_Rewrite_Range
> BTND_RewriteData;

enum {
	BTND_Rewrite_None_TYPE,
	BTND_Rewrite_Literal_TYPE,
	BTND_Rewrite_Number_TYPE,
	BTND_Rewrite_Variable_TYPE,
	BTND_Rewrite_Function_TYPE,
	BTND_Rewrite_DateTime_TYPE,
	BTND_Rewrite_Range_TYPE
}; 

/// when updating the enums make sure to sunc up BTNDDecode::typeName_XXX 

/// pattern structure flow data - blank for now
struct BTND_StructData {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_LIST, // sequence of elements
		T_ANY,  // any element 
		T_OPT,  // optional subtree
		T_PERM, // permutation of children
		T_TAIL, // if children are A,B,C this translates into [A], [A,B] and [A,B,C]
		/// add new types above this line only
		BEL_STRUCT_MAX
	};
	uint8_t type;
	BTND_StructData() : type(T_LIST) {}
	BTND_StructData(int t) : type(t) {}
};

/// blank data type
struct BTND_None {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
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

//// tree of these nodes is built during parsing
//// a the next step the tree gets interpreted into BrzelTrie path and added to a trie 
struct BELParseTreeNode {
	/// see barzer_el_btnd.h for the structure of the nested variant 
	BTNDVariant btndVar;  // node data

	typedef std::vector<BELParseTreeNode> ChildrenVec;

	ChildrenVec child;
	
	template <typename T>
	BELParseTreeNode& addChild( const T& t) {
		child.resize( child.size() +1 );
		child.back().btndVar = t;
		return child.back();
	}

	BTNDVariant& getVar() { return btndVar; }
	void clear( ) 
	{
		child.clear();
		btndVar = BTND_None();
	}

	template <typename T>
	T& setNodeData( const T& d ) { return(btndVar=d, boost::get<T>(btndVar));}
	
	template <typename T>
	void getNodeDataPtr_Pattern( T*& ptr ) 
	{ 
		if( btndVar.which() == BTND_PatternData_TYPE ) 
			ptr = boost::get<T>(&(boost::get<BTND_PatternData>(btndVar)));
		else 
			ptr = 0;
	}
	template <typename T>
	void getNodeDataPtr_Struct( T*& ptr ) 
		{ 
		if( btndVar.which() == BTND_StructData_TYPE ) 
			ptr = boost::get<T>(&(boost::get<BTND_StructData>(btndVar)));
		else 
			ptr = 0;
		}

	template <typename T>
	void getNodeDataPtr_Rewrite( T*& ptr ) 
		{ 
		if( btndVar.which() == BTND_RewriteData_TYPE ) 
			ptr = boost::get<T>(&(boost::get<BTND_RewriteData>(btndVar)));
		else 
			ptr = 0;
		}
	
	void print( std::ostream& fp, int depth=0 ) const;
	bool isChildless() const
		{ return ( !child.size() ); }
	
	/// if this subtree is effectively a single rewrite we will return a pointer to the node that 
	/// actually performs the rewrite
	const BELParseTreeNode* getTrivialChild() const
	{
		if( !child.size() ) 
			return this;
		else if( child.size() == 1 )
			return (child.front().isChildless() ? &(child.front()) : 0);
		else
			return 0;
	}
	const BTNDVariant& getNodeData() const 
		{ return btndVar; }
	
	const BTND_RewriteData* getRewriteData() const
		{ return( boost::get<BTND_RewriteData>( &btndVar) ); }
	const BTND_StructData* getStructData() const
		{ return( boost::get<BTND_StructData>( &btndVar) ); }
	const BTND_PatternData* getPatternData() const
		{ return( boost::get<BTND_PatternData>( &btndVar) ); }
	
	const BTND_RewriteData* getTrivialRewriteData() const
	{
		const BELParseTreeNode* tn = getTrivialChild();
		return( tn ? tn->getRewriteData() : 0 );
	}
};

//struct PatternEmitterNode;
struct PatternEmitterNode {
    virtual bool step() = 0;
    virtual void yield(BTND_PatternDataVec& vec) const = 0;
    virtual ~PatternEmitterNode() {}

    static PatternEmitterNode* make(const BELParseTreeNode& node);
};

struct BELParseTreeNode_PatternEmitter {
	BTND_PatternDataVec curVec;

	const BELParseTreeNode& tree;
	
	BELParseTreeNode_PatternEmitter( const BELParseTreeNode& t ) : tree(t)
        { makePatternTree(); }

	/// returns false when fails to produce a sequence
	bool produceSequence();
	const BTND_PatternDataVec& getCurSequence( )
	{
		patternTree->yield(curVec);
		return curVec;
	}

	
	~BELParseTreeNode_PatternEmitter();
private:
    PatternEmitterNode* patternTree;
    void makePatternTree();
};

} // barzer namespace

#endif // BARZER_EL_BTND_H