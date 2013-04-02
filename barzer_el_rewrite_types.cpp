#include <barzer_el_rewrite_types.h>
#include <barzer_universe.h>

namespace barzer {

std::ostream&  BTND_Rewrite_Variable::print( std::ostream& fp , const BELPrintContext& ctxt ) const
{
	switch( idMode ) {
	case MODE_WC_NUMBER: // wildcard number
		return( fp << "$" << varId  );
	case MODE_VARNAME:   // variable name 
        fp <<  "$" ;
        ctxt.printVariableName( fp, varId ) ;
		return fp;
	case MODE_PATEL_NUMBER: // pattern element number
		return( fp << "p$" << varId );
	}
	return ( fp << "bad$" << varId);
}
std::ostream&  BTND_Rewrite_Function::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
	return ( fp << "func." << ctxt.printableString( nameId ) );
}


std::ostream&  BTND_Rewrite_Select::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
    return ( fp << "select." << ctxt.printableString( varId ) );
}

std::ostream&  BTND_Rewrite_Case::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
    return ( fp << "case." << ctxt.printableString( ltrlId ) );
}

std::ostream&  BTND_Rewrite_Logic::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
    switch(type) {
    case AND:
        fp << "AND";
    case OR:
        fp << "OR";
    case NOT:
        fp << "NOT";
    }
    return fp;
}

std::ostream&  BTND_Rewrite_None::print( std::ostream& fp , const BELPrintContext& ) const
{
	return (fp << "RwrNone" );
}

namespace {

typedef boost::variant<int,std::string> shit_var_t;

struct BTND_RewriteData_equal_vis : public boost::static_visitor<bool> 
{
    const BTND_RewriteData& dta;
    BTND_RewriteData_equal_vis( const BTND_RewriteData& x ) : dta(x) {}

    template <typename T>
    bool operator()( const T& o ) const
        { return o == boost::get<T>( dta ); }

    bool isEqual( const BTND_RewriteData& o ) const
    {
        if( o.which() == dta.which() ) 
            return boost::apply_visitor( *this, o );
        else
            return false;
    }
};

}

bool BTND_RewriteData_equal( const BTND_RewriteData& l, const BTND_RewriteData& r )
{
    if( l.which() == r.which() ) {
        BTND_RewriteData_equal_vis vis( l );
        return vis.isEqual( r );
    }
    return false;
}
} // namespace barzer
