#include <barzer_el_rewrite_control.h>
#include <barzer_barz.h>
#include <barzer_universe.h>
#include <barzer_el_rewriter.h>
#include <sstream>

namespace barzer {

namespace {

void pushControlError(BarzelEvalContext& ctxt, const char* ctrlName, const char* error, const char* sig=0 )
{
    std::stringstream ss;
    ss << "<ctrlerr";
    if( ctrlName ) 
        ss << " c=\"" << ( ctrlName ? ctrlName: "" );
    ss << "\">" << ( error ? error: "" ) ;

    if( sig ) {
        ss << "<sig>" << sig << "</sig>";
    }
    ss << "</ctrlerr>";

    ctxt.getBarz().barzelTrace.pushError( ss.str().c_str() );
}

}

bool BarzelControlEval::operator()( BarzelEvalResult& val, const BTND_Rewrite_Control& ctrl )
{
    const GlobalPools& gp = d_ctxt.getUniverse().getGlobalPools();
    switch( ctrl.getCtrl() ) {
    case BTND_Rewrite_Control::RWCTLT_VAR_GET:{
            const BarzelEvalResult* varResult = d_ctxt.getVar( ctrl.getVarId()  );
            if( varResult ) {
                val = *varResult;
                return true;
            } else {
                pushControlError( d_ctxt, "Var", "no such variable", gp.internalString_resolve(ctrl.getVarId()) );
                return false;
            }
        }break;
    default:
        pushControlError( d_ctxt, 0, "unknown control structure" );
        break;
    }

    return false;
}

}  // namespace barzer
