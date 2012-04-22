#ifndef BARZER_EL_REWRITE_CONTROL_H
#define BARZER_EL_REWRITE_CONTROL_H
//// evaluation of control statements for rewrite  
class BarzelEvalResultVec;
namespace barzer {

class BarzelEvalContext;
class GlobalPools;

class BarzelEvalResult;
class BTND_Rewrite_Control;
class BarzelEvalNode;

class BarzelControlEval {
    BarzelEvalContext& d_ctxt;
	const BarzelEvalResultVec& d_childValVec;
	const BarzelEvalNode &d_evalNode;
public:
    BarzelControlEval( BarzelEvalContext& ctxt, const BarzelEvalResultVec& childValVec, const BarzelEvalNode& evalNode ) :
        d_ctxt(ctxt) ,
        d_childValVec(childValVec),
        d_evalNode( evalNode )
    {}
    bool operator()( BarzelEvalResult& val, const BTND_Rewrite_Control& ctrl );
};

} // namespace barzer
#endif //  BARZER_EL_REWRITE_CONTROL_H
