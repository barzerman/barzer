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

} // namespace barzer
