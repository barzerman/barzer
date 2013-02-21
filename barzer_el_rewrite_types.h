/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_basic_types.h>
#include <barzer_basic_types_range.h>

namespace barzer {

//// REWRITE Types
/// translation is essentially a tree each node having one and only 
/// one rewrite type 
/// 
struct BTND_Rewrite_Literal : public  BarzerLiteral {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
		{ return BarzerLiteral::print( fp, ctxt ); }
};

struct BTND_Rewrite_Number : public BarzerNumber {
	
	uint8_t isConst; /// when false we will need to evaluate underlying nodes

	BTND_Rewrite_Number( ) : isConst(0) {}

	BTND_Rewrite_Number( int x ) : BarzerNumber((int64_t)x), isConst(0) {}
	BTND_Rewrite_Number( double x ) : BarzerNumber(x), isConst(0) {}

	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerNumber::print( fp ); }

	void set( int64_t i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set( int i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set_int( int i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set( double i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set_double( double i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }

	void setBarzerNumber( BarzerNumber& x ) const
	{
		x = *(static_cast<const BarzerNumber*>(this));
	}
};
/// this is a runtime rewrite - potentially large type 
/// it's not stored but constructed on the fly in the matcher
struct BTND_Rewrite_RuntimeEntlist {
    BarzerEntityList lst;
};
struct BTND_Rewrite_MkEnt {
	uint32_t d_entId;
	/// when mode is 0 d_entId is the single entity Id 
	//  when mode is 1 d_entId is id of the pool of entityIds 
	enum {
		MODE_SINGLE_ENT,
		MODE_ENT_LIST
	};
	uint8_t  d_mode;


	BTND_Rewrite_MkEnt() : d_entId(0xffffffff), d_mode(MODE_SINGLE_ENT) {}
	
	bool isValid() const { return (d_entId != 0xffffffff); }
	bool isSingleEnt() const { return d_mode == MODE_SINGLE_ENT; }
	bool isEntList() const { return d_mode == MODE_ENT_LIST; }

	// single entity manipulation
	void setEntId( uint32_t i ) { 
		d_entId = i; 
		d_mode = MODE_SINGLE_ENT;
	}
	uint32_t getRawId( ) const { return d_entId; }
	uint32_t getEntId( ) const { return ( isSingleEnt() ? d_entId : 0xffffffff ); }

	// entity list manipulation
	void setEntGroupId( uint32_t i ) {
		d_entId = i; 
		d_mode = MODE_ENT_LIST;
	}

	uint32_t getEntGroupId( ) const { return isEntList() ? d_entId :0xffffffff; }

	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return (fp<< std::hex << d_entId ); }
};

struct BTND_Rewrite_Variable {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		MODE_WC_NUMBER, // wildcard number
		MODE_VARNAME,   // variable name 
		MODE_PATEL_NUMBER, // pattern element number
		MODE_WCGAP_NUMBER, // wildcard gap number
		MODE_POSARG, // positional argument number
        MODE_REQUEST_VAR /// variable lives in RequestEnvironment::d_reqVar
	};
	uint8_t idMode; 
	uint32_t varId; /// when idMode is MODE_VARNAME this is a variable path id (see el_variables.h)
		// otherwise as $1/$2 

	bool isValid() const { return varId != 0xffffffff; }

	uint8_t getIdMode() const { return idMode; }
	uint32_t getVarId() const { return varId; }
		
	bool isPosArg() const { return idMode == MODE_POSARG; }
	bool isRequestVar() const { return idMode == MODE_REQUEST_VAR; }
	bool isVarId() const { return idMode == MODE_VARNAME; }
	bool isWildcardNum() const { return idMode == MODE_WC_NUMBER; }
	bool isPatternElemNumber() const { return idMode == MODE_PATEL_NUMBER; }
	bool isWildcardGapNumber() const { return idMode == MODE_WCGAP_NUMBER; }

	void setPosArg( uint32_t vid ){ varId = vid; idMode= MODE_POSARG; }
	void setVarId( uint32_t vid ){ varId = vid; idMode= MODE_VARNAME; }
	void setWildcardNumber( uint32_t vid ){ varId = vid; idMode= MODE_WC_NUMBER; }
	void setPatternElemNumber( uint32_t vid ){ varId = vid; idMode= MODE_PATEL_NUMBER; }
	void setWildcardGapNumber( uint32_t vid ){ varId = vid; idMode= MODE_WCGAP_NUMBER; }
	void setRequestVarId( uint32_t vid ){ varId = vid; idMode= MODE_REQUEST_VAR; }
	void changeToReqVar( ){ idMode= MODE_REQUEST_VAR; }

	BTND_Rewrite_Variable() : 
		idMode(MODE_WC_NUMBER), 
		varId(0xffffffff) 
	{}
};

struct RewriteVariableMode {
};
typedef enum {
    VARMODE_REWRITE,// variable local to a single rewrite (DEFAULT)
    VARMODE_REQUEST // variable local to the entire request RequestEnvironment::d_reqVar RequestVariableMap
} Btnd_Rewrite_Varmode_t;

struct BTND_Rewrite_Function {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint32_t nameId; // function name id
	uint32_t argStrId; // string id for the optional arg attribute
    uint32_t varId;    // string id for a variable (optional)

    uint8_t  d_varMode; // VARMODE_XXX 

	void setNameId( uint32_t i ) { nameId = i ; }
	uint32_t getNameId() const { return nameId; }

    void setArgStrId( uint32_t i ) { argStrId = i; }
    uint32_t getArgStrId( ) const { return argStrId; }

	BTND_Rewrite_Function() : 
        nameId(ay::UniqueCharPool::ID_NOTFOUND),
        argStrId(ay::UniqueCharPool::ID_NOTFOUND) ,
        varId(ay::UniqueCharPool::ID_NOTFOUND) ,
        d_varMode(VARMODE_REWRITE)
    {}

	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id) : 
        nameId(id), 
        argStrId(ay::UniqueCharPool::ID_NOTFOUND)  ,
        varId(ay::UniqueCharPool::ID_NOTFOUND)  ,
        d_varMode(VARMODE_REWRITE)
    {}

	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id, ay::UniqueCharPool::StrId argId) : 
        nameId(id), argStrId(argId) , varId(ay::UniqueCharPool::ID_NOTFOUND),d_varMode(VARMODE_REWRITE)
    {}
    
    void        setVarId( uint32_t v ) { varId= v; }
    uint32_t    getVarId() const {return varId; }
    bool        isValidVar() const { return (varId != ay::UniqueCharPool::ID_NOTFOUND); }

    void setVarModeRequest( ) { d_varMode=VARMODE_REQUEST; }
    bool isReqVar() const    { return (d_varMode== VARMODE_REQUEST); }
    void setVarModeRewrite( ) { d_varMode=VARMODE_REWRITE; } // default
    bool isRewriteVar() const    { return (d_varMode== VARMODE_REWRITE); }
};

struct BTND_Rewrite_Select {
    uint32_t varId;
    void setVarId(uint32_t id) { varId = id; }
    uint32_t getVarId() const { return varId; }

    BTND_Rewrite_Select(uint32_t vid = 0xffffffff)
        : varId(vid) {}

    std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};


struct BTND_Rewrite_Case {
    uint32_t ltrlId;
    BTND_Rewrite_Case(uint32_t id = 0xffffffff) : ltrlId(id) {}

    std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};

struct BTND_Rewrite_Logic {
    enum Type { AND, OR, NOT  };
    Type type;
    Type getType() const { return type; }
    void setType(Type t)
        { type = t; }
    BTND_Rewrite_Logic(): type(AND) {}
    std::ostream& print( std::ostream&, const BELPrintContext& ) const;
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

/// only makes sense when it's underneath BTND_Rewrite_EntitySearch_Srch
/// each TND_Rewrite_EntitySearch_Srch can have multiple class filters 
struct BTND_Rewrite_EntitySearch_ClassFilter {
	// 0xffff is a wilcdard (meaning any class or subclass would do)
	uint16_t d_entClass, d_entSubclass;

	BTND_Rewrite_EntitySearch_ClassFilter() : d_entClass(0xffff), d_entSubclass(0xffff) {}
	BTND_Rewrite_EntitySearch_ClassFilter( uint16_t c, uint16_t sc) : 
		d_entClass(c), d_entSubclass(sc) 
	{}

	std::ostream& print( std::ostream& fp ) const
		{ return fp << std::hex << d_entClass << ":" << std::hex << d_entSubclass; }
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return print(fp); }
};

/// runs entity search based on subordinate BTND_Rewrite_EntitySearch_Arg_XXX nodes
struct BTND_Rewrite_EntitySearch_Srch {
	std::ostream& print( std::ostream& fp ) const
		{ return fp << "EntSearch"; }
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return print(fp); }
};

typedef boost::variant<
	BTND_Rewrite_EntitySearch_ClassFilter,
	BTND_Rewrite_EntitySearch_Srch
> BTND_Rewrite_EntitySearch;

typedef boost::variant<
	BTND_Rewrite_AbsoluteDate,
	BTND_Rewrite_TimeOfDay
> BTND_Rewrite_DateTime;

/// control structure - BLOCK/LOOP etc 
/// implementation will start with block 
/// var=control(...) 
struct BTND_Rewrite_Control {
    enum { 
        RWCTLT_COMMA,  // evaluates all children returns the vlue of the last
        RWCTLT_LIST,  // evaluates all children returns everything every child produces
        // init is like a scoped declaration in C++ 
        // let is like X=y in javascript - if no surrounding context has X , X will be created 
        RWCTLT_VAR_BIND, // assigns (binds) variable - tries to find it in current context and if fails - creates
        RWCTLT_VAR_GET,  // extracts variable - tries all ascending contexts 
        
        /// add new ones above this line
        RWCTLT_MAX
    }; 
    uint16_t d_rwctlt; // RWCTLT_XXXX
    uint32_t d_varId;  //  default 0xffffffff - when set results will be also added to the variable 
    uint8_t  d_varMode;

    void setVarModeRequest( ) { d_varMode=VARMODE_REQUEST; }
    bool isReqVar() const    { return (d_varMode== VARMODE_REQUEST); }
    void setVarModeRewrite( ) { d_varMode=VARMODE_REWRITE; } // default
    bool isRewriteVar() const    { return (d_varMode== VARMODE_REWRITE); }
    BTND_Rewrite_Control() : d_rwctlt(RWCTLT_COMMA), d_varId(0xffffffff), d_varMode(VARMODE_REWRITE) {}
    /// here 4 more bytes of something could be used  
	std::ostream& print( std::ostream& fp ) const
		{ return fp << "Control(" << d_rwctlt << "," << d_varId << ")"; }
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return print(fp); }
    
    bool isValidVar() const { return d_varId != 0xffffffff; }

    void setVarId( uint32_t i ) { d_varId= i; }
    uint32_t getVarId() const { return d_varId; }

    uint16_t getCtrl() const { return d_rwctlt; }
    void setCtrl( uint32_t m )
    {
        if( m>= RWCTLT_COMMA && m< RWCTLT_MAX ) 
            d_rwctlt = static_cast<uint16_t>( m );
    }
    bool isCtrl( uint32_t m ) 
        { return ( m == d_rwctlt ); }
    bool isComma() const { return( d_rwctlt == RWCTLT_COMMA ); }
};

typedef boost::variant< 
	BTND_Rewrite_None,
	BTND_Rewrite_Literal,
	BTND_Rewrite_Number,
	BTND_Rewrite_Variable,
	BTND_Rewrite_Function,
	BTND_Rewrite_DateTime, // needs to be removed
	BTND_Rewrite_Range, // needs to be removed
	BTND_Rewrite_EntitySearch, // needs to be removed
	BTND_Rewrite_MkEnt,
	BTND_Rewrite_Select,
	BTND_Rewrite_Case,
	BTND_Rewrite_Logic,
    BTND_Rewrite_Control,
    BTND_Rewrite_RuntimeEntlist
> BTND_RewriteData;

enum {
	BTND_Rewrite_None_TYPE,
	BTND_Rewrite_Literal_TYPE,
	BTND_Rewrite_Number_TYPE,
	BTND_Rewrite_Variable_TYPE,
	BTND_Rewrite_Function_TYPE,
	BTND_Rewrite_DateTime_TYPE,
	BTND_Rewrite_Range_TYPE,
	BTND_Rewrite_EntitySearch_TYPE,
	BTND_Rewrite_MkEnt_TYPE,
	BTND_Rewrite_Select_TYPE,
	BTND_Rewrite_Case_TYPE,
	BTND_Rewrite_Logic_TYPE,
	BTND_Rewrite_Control_TYPE,
    BTND_Rewrite_RuntimeEntlist_TYPE
}; 

} // namespace barzer
