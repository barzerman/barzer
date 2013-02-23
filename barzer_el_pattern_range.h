#pragma once
#include <barzer_el_pattern.h>
#include <barzer_basic_types_range.h>
namespace barzer {

class BTND_Pattern_Range : public BTND_Pattern_Base {
protected:
	enum {
		MODE_TYPE, // only compares the range type 
		MODE_VAL // compares the range value
	};
	BarzerRange d_range;
	uint8_t d_mode; 

    enum {
        FLAVOR_NORMAL,
        FLAVOR_NUMERIC // when this flavor is set match operator will looks at int and real
    };
	uint8_t d_rangeFlavor; 
public:
	void setModeToType() { d_mode = MODE_TYPE; }
	void setModeToVal() { d_mode = MODE_VAL; }

	bool isModeVal() const { return( MODE_VAL == d_mode) ; }

	BarzerRange& range() { return d_range; }
	const BarzerRange& range() const { return d_range; }

	BTND_Pattern_Range( ) : 
        d_mode(MODE_TYPE) ,
        d_rangeFlavor(FLAVOR_NORMAL)
    {}
	BTND_Pattern_Range( const BarzerRange& r) : 
		d_range(r),
		d_mode(MODE_TYPE),
        d_rangeFlavor(FLAVOR_NORMAL)
	{}
    bool isFlavorNumeric() const { return(d_rangeFlavor == FLAVOR_NORMAL); }
    void setFlavorNumeric() { d_rangeFlavor = FLAVOR_NUMERIC; }

	bool isEntity() const { return d_range.isEntity(); }
	void setEntityClass( const StoredEntityClass& c ) 
	{ d_range.setEntityClass(c); }
	bool lessThan( const BTND_Pattern_Range& r ) const { 
        return( d_mode< r.d_mode ? true :
            (r.d_mode < d_mode ? false : d_range< r.d_range) 
        );
	}
	bool operator()( const BarzerRange&) const;
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const ;
	std::ostream& print( std::ostream& fp,const BELPrintContext& ) const 
	{ return (fp << '[' << d_range) << ']'; }
};
inline bool operator< ( const BTND_Pattern_Range& l, const BTND_Pattern_Range& r ) 
{ return l.lessThan(r); }

}
