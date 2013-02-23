/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <stdint.h>
#include <ay/ay_bitflags.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util.h>
#include <boost/variant.hpp>
#include <barzer_basic_types.h>
#include <barzer_el_variable.h>
#include <barzer_el_pattern.h>
#include <barzer_el_pattern_number.h>
#include <barzer_el_pattern_entity.h>
#include <barzer_el_pattern_datetime.h>
#include <barzer_el_pattern_token.h>
#include <barzer_el_pattern_range.h>
#include <barzer_el_rewrite_types.h>

// BTND stands for Barzel Trie Node Data
/// 
// types defined in this file are used to store information for various node types 
// in the BarzEL trie
namespace barzer {
class GlobalPools;
struct BELPrintContext;
class BELTrie;

/// blank data type 
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
		BTND_Pattern_StopToken,		 	// 9
		BTND_Pattern_Entity,		    // 10
		BTND_Pattern_ERCExpr,			// 11
		BTND_Pattern_ERC,			    // 12
		BTND_Pattern_Range,				// 13
		BTND_Pattern_Meaning,			// 14
		BTND_Pattern_EVR			    // 15
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
	BTND_Pattern_Entity_TYPE,           // 10
	BTND_Pattern_ERCExpr_TYPE,          // 11
	BTND_Pattern_ERC_TYPE,          // 12
	BTND_Pattern_Range_TYPE,          // 13
	BTND_Pattern_Meaning_TYPE,          // 14
	BTND_Pattern_EVR_TYPE,              // 15


	/// end of wildcard types - add new ones ONLY ABOVE THIS LINE
	/// in general there should be no more than 1-2 more types added 
	/// nest them in another variant 
	BTND_Pattern_MAXWILDCARD_TYPE,
	
	BTND_Pattern_TYPE_MAX
};

struct BTND_Pattern_TypeId_Resolve {
	template <typename T> int operator() () const { return BTND_Pattern_None_TYPE; }

};
template <> inline int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_None > () const { return  BTND_Pattern_None_TYPE; }
template <> inline int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Token > ( ) const { return  BTND_Pattern_Token_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Punct  > ( ) const { return  BTND_Pattern_Punct_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_CompoundedWord > ( ) const { return  BTND_Pattern_CompoundedWord_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Number > ( ) const { return  BTND_Pattern_Number_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()<BTND_Pattern_Wildcard > ( ) const { return  BTND_Pattern_Wildcard_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Date > ( ) const { return  BTND_Pattern_Date_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Time> ( ) const { return  BTND_Pattern_Time_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_DateTime> ( ) const { return  BTND_Pattern_DateTime_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_StopToken> ( ) const { return  BTND_Pattern_StopToken_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Entity> ( ) const { return  BTND_Pattern_Entity_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Range> ( ) const { return  BTND_Pattern_Range_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_ERCExpr> ( ) const { return  BTND_Pattern_ERCExpr_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_ERC> ( ) const { return  BTND_Pattern_ERC_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_EVR> ( ) const { return  BTND_Pattern_EVR_TYPE; }

/// pattern tyepe number getter visitor 

typedef std::vector< BTND_PatternData > BTND_PatternDataVec;

/// when updating the enums make sure to sunc up BTNDDecode::typeName_XXX 

/// pattern structure flow data - blank for now
struct BTND_StructData {
	enum {
		T_LIST, // sequence of elements
		T_ANY,  // any element 
		T_OPT,  // optional subtree
		T_PERM, // permutation of children
		T_TAIL, // if children are A,B,C this translates into [A], [A,B] and [A,B,C]

		T_SUBSET, // if children are A,B,C this translates into A,B,C,AB,AC,BC,ABC
        T_FLIP,   // ABCD = (ABCD,DCBA)
		/// add new types above this line only

		BEL_STRUCT_MAX
	};
protected:
	uint32_t varId;  // variable id default 0xffffffff
	uint8_t type;
public:
	const char* getXMLTag( ) const { 
		switch( type ) {
		case T_LIST: return "list";
		case T_ANY: return "any";
		case T_OPT: return "opt";
		case T_PERM: return "perm";
		case T_TAIL: return "tail";
		case T_SUBSET: return "subs";
		case T_FLIP: return "perm";
		default: return "badstruct";
		}
	}

	std::ostream& printXML( std::ostream&, const BELTrie& ) const;
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;

	int getType() const { return type; }
	void setType( int t ) { type = t; }

	uint32_t getVarId() const { return varId; }
		void setVarId( uint32_t vi ) { varId = vi; }
	
	const inline bool hasVar() const { return varId != 0xffffffff; }

	BTND_StructData() : varId(0xffffffff), type(T_LIST) {}
	BTND_StructData(int t) : varId(0xffffffff), type(t) {}
	BTND_StructData(int t, uint32_t vi ) : varId(vi), type(t) {}

    bool isANY() const { return type == T_ANY; }
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

class BELTrie;

//// tree of these nodes is built during parsing
//// a the next step the tree gets interpreted into BrzelTrie path and added to a trie 
struct BELParseTreeNode {
	BTNDVariant btndVar;  // node data

	typedef std::vector<BELParseTreeNode> ChildrenVec;

    uint8_t noTextToNum;

	ChildrenVec child;
    bool isStruct() const { return btndVar.which() == BTND_StructData_TYPE; }
	/// translation nodes may have things such as <var name="a.b.c"/>
	/// they get encoded during parsing and stored in BarzelVariableIndex::d_pathInterner
    
	BELParseTreeNode():noTextToNum(0) {}

	template <typename T>
	BELParseTreeNode& addChild( const T& t)
	{
		child.resize( child.size() +1 );
		child.back().btndVar = t;
		return child.back();
	}

	BELParseTreeNode& addChild( const BELParseTreeNode& node ) 
	{
		child.resize( child.size() +1 );
		return( child.back() = node, child.back() );
	}

	const BTNDVariant& getVar() const { return btndVar; }
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
	      BTNDVariant& getNodeData()
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

	/// streams out XML recursively. the result should look like regular barzel xml 
	std::ostream& printBarzelXML( std::ostream& fp, const BELTrie&  ) const;

    // tries to produce one of the names from pattern 
    // treats every struct element as a list except for ANY from which only the first element will be extracted  
    // this is *likely* to produce a decent name. This is guaranteed to be very fast
    void getDescriptiveNameFromPattern_simple( std::string& , const GlobalPools& u );
};

/// barzel macros 
class BarzelMacros {
	typedef std::map< uint32_t, BELParseTreeNode >  MacroMap;
	MacroMap d_macroMap;
	
public:
	BELParseTreeNode& addMacro(  uint32_t  macro ) 
		{ return d_macroMap[ macro ]; }
	const BELParseTreeNode* getMacro( uint32_t macro ) const
	{
		MacroMap::const_iterator i = d_macroMap.find( macro );
		return ( i == d_macroMap.end() ? 0: &(i->second) );
	}
    void clear() { d_macroMap.clear(); }
};

} // barzer namespace
