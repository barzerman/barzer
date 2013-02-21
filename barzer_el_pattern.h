/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <stdint.h>
#include <iostream>
#include <ay/ay_util.h>
#include <barzer_basic_types.h>

//// BarzelTrieNodeData - PATTERN types 
/// all pattern classes must inherit from this 
namespace barzer {
struct BELPrintContext;
struct BTND_Pattern_Base {
    enum {
        MATCH_MODE_NORMAL=0, 
        MATCH_MODE_NEGATIVE,
        MATCH_MODE_FUZZY,
        MATCH_MODE_FUZZY_NEGATIVE

        /// cant be greater than 7
    };
    uint8_t d_matchMode; // 0 - straight equality match, 1 - negat
    /// match operator 
    template <typename T> bool operator()( const T& t ) const { return false; }
    BTND_Pattern_Base() : d_matchMode(MATCH_MODE_NORMAL) {}
    
    void mkPatternMatch_Normal() { d_matchMode= MATCH_MODE_NORMAL; }
    void mkPatternMatch_Negative() { d_matchMode= MATCH_MODE_NEGATIVE; }
    void mkPatternMatch_Fuzzy() { d_matchMode= MATCH_MODE_FUZZY; }
    void mkPatternMatch_FuzzyNegative() { d_matchMode= MATCH_MODE_FUZZY_NEGATIVE; }

    void setMatchModeFromAttribute( const char* v ) 
    {
        switch(*v) {
        case 'n': mkPatternMatch_Negative(); break;
        case 'f': mkPatternMatch_Fuzzy(); break;
        case 'g': mkPatternMatch_FuzzyNegative(); break;
        }
    }
};

struct BTND_Pattern_None : public BTND_Pattern_Base{
	std::ostream& print( std::ostream& fp , const BELPrintContext& ) const { return fp << "PatNone"; } 
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_None& x )
	{ return( fp << "<none>" ); }

// simple token wildcard data
/// no attributes - matches anything
struct BTND_Pattern_Wildcard : public BTND_Pattern_Base {
    enum { 
        WT_ANY, // match any bead (default)
        WT_ENT  // entity related stuff (erc, ent ...)
    };
    uint32_t d_type;

    BTND_Pattern_Wildcard() : d_type(WT_ANY) {}

    uint32_t getType() const { return d_type; } 

    bool isType( uint32_t x ) const {
        return (d_type==x);
    }
	bool isLessThan( const BTND_Pattern_Wildcard& r ) const
        { return (d_type< r.d_type); }
    void setFromAttr( const char* v ) {
        if(v) {
            switch( *v ) {
            case 'a': d_type= WT_ANY; break; 
            case 'e': d_type= WT_ENT; break; 
            }
        }
    }
    std::ostream& print( std::ostream& fp ) const { return (fp << d_type); }
    std::ostream& print( std::ostream& fp, const BELPrintContext&   ) const 
    { return print(fp); }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Wildcard& x )
{ 
        return x.print(fp);
}
inline bool operator <( const BTND_Pattern_Wildcard& l, const BTND_Pattern_Wildcard& r )
	{ return l.isLessThan( r ); }

} // namespace barzer
