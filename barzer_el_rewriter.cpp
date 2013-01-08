#include <barzer_el_rewriter.h>
#include <barzer_el_function.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <barzer_el_matcher.h>
#include <ay/ay_logger.h>
#include <barzer_el_rewrite_control.h>
#include <barzer_server.h>

//#include <boost/foreach.hpp>

namespace barzer {

void BarzelRewriterPool::clear()
{
	for( BufAndSizeVec::iterator i = encVec.begin(); i!= encVec.end(); ++i ) 
		free((uint8_t*)(i->first));
}

BarzelRewriterPool::~BarzelRewriterPool()
{
	clear();
}

namespace {


struct Fallible {
	bool isThatSo;

	bool nodeStart() { return true; }
	bool nodeEnd() { return true; }
	bool nodeData( const BTND_RewriteData& d ) {
		const BTND_Rewrite_EntitySearch* srch = boost::get<BTND_Rewrite_EntitySearch>( &d);
		if( srch ) {
			return ( isThatSo= false );
		} else 
			return true;
	}
	Fallible() : isThatSo(false) {}
};

// Comparator for sorting a list of <case> nodes. Has the following effect:
// all <case> node are moved to the beginning of the list and sorted
// the rest is put to the end of the list in original order
// assuming std::stable_sort is called
// so it probably can be called for any list safely
struct CaseComparator : public boost::static_visitor<bool> {
    bool operator()(const BELParseTreeNode &l, const BELParseTreeNode &r) const
    {
        return boost::apply_visitor(*this, *l.getRewriteData(), *r.getRewriteData());
    }
    bool operator()(const BTND_RewriteData &l, const BTND_RewriteData &r) const
        {
        return boost::apply_visitor(*this, l, r);
        }
    bool operator()(const BTND_Rewrite_Case &l, const BTND_Rewrite_Case &r) const
        { return l.ltrlId < r.ltrlId; }
    template<class T> bool operator()(const T&, const BTND_Rewrite_Case &l) const
        { return false; }
    template<class T,class U> bool operator()(const T&, const U&) const
        { return true; }
};



struct CaseFinder : public boost::static_visitor<bool> {
    uint32_t id;
    CaseFinder(uint32_t i) : id(i) {}
    bool operator()(const BarzelEvalNode &l, uint32_t i) {
        return boost::apply_visitor(*this, l.getBtnd());
    }

    template<class T> bool operator()(const T&) {
        AYLOG(DEBUG) << "unknown";
        return false;
     }
};

template<> bool CaseFinder::operator()<BTND_Rewrite_Case>(const BTND_Rewrite_Case &l)
    { return l.ltrlId < id; }

} // anon namespace ends 

bool  BarzelRewriterPool::isRewriteFallible( const BarzelRewriterPool::BufAndSize& bas ) const 
{
	Fallible fallible;
	BarzelRewriteByteCodeProcessor<Fallible> processor(fallible,bas);
	return fallible.isThatSo;
}

int  BarzelRewriterPool::encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn ) 
{
	// for every node push node_start_byte, then BTND_Rewrite (the whole variant) (otn.getNodeData() ) - memcpy, then node end byte

	trans.push_back( (uint8_t)barzel::RWR_NODE_START );
	//std::cerr << "(";
	const BTND_RewriteData* rd = ptn.getRewriteData();
	if( !rd ) 
		return ERR_ENCODING_FAILED;

	const uint8_t* rdBuf = (const uint8_t*) rd;
	size_t dta_sz = sizeof(BTND_RewriteData);
	trans.insert( trans.end(), rdBuf, rdBuf+dta_sz);
	//std::cerr << "BTND_RewriteData[" << rd->which() << ":" << dta_sz << "]";

	BELParseTreeNode::ChildrenVec child = ptn.child;
	if (rd->which() == BTND_Rewrite_Select_TYPE) {
	    /*size_t len = child.size();
	    std::vector<const BELParseTreeNode*> v(len, 0);
	    for (size_t i = 0, s = child.size(); i < s; ++i)
	        v[i] = &child[i]; */
	    // this is probably insanely inefficient, will need to correct
	    std::stable_sort(child.begin(), child.end(), CaseComparator());
	}

	for( BELParseTreeNode::ChildrenVec::const_iterator ch = child.begin(),
	                                                       chend = child.end();
	                                                ch != chend; ++ch ) {
		int rc =encodeParseTreeNode( trans, *ch );
		if( rc ) 
			return rc; // error
	}
	trans.push_back( (uint8_t)barzel::RWR_NODE_END );
	//std::cerr << ")";
	return ERR_OK;
}

bool BarzelRewriterPool::resolveTranslation( BarzelRewriterPool::BufAndSize& bas, const BarzelTranslation& trans ) const
{
	uint32_t rid = trans.getRewriterId( );
	if( rid < encVec.size() ) 
		return ( bas = encVec[rid], true );
	else 
		return ( bas = BarzelRewriterPool::BufAndSize(0,0), false );
		
}

int BarzelRewriterPool::produceTranslation( BarzelTranslation& trans, const BELParseTreeNode& ptn )
{
	byte_vec enc;
	if( !encodeParseTreeNode(enc,ptn) ) {
		if( enc.size() ) {
			uint32_t rewrId = poolNewBuf( &(enc[0]), enc.size() );
			trans.setRewriter( rewrId );
			return ERR_OK;
		} else {
			AYTRACE( "inconsistent encoding produced");
			return ERR_ENCODING_INCONSISTENT;
		}
	} else
			return ERR_ENCODING_FAILED;
}

//// barzel evaluator (translator)
namespace {


// visitor to check if we should eval the child
// some of the children need not to be be untill much later (case/if)
struct Eval_visitor_evalChildren :  public boost::static_visitor<bool> {
    bool operator()(const BTND_Rewrite_Select&) const
    { return false; }
    template <typename T>
    bool operator() ( const T& btnd ) const
        { return true; }
};


/// this visitor would only make sense for operators that may need less than 
/// all of their dependent values computd. this primarily relates to 
/// control structures 
struct Eval_visitor_needToStop : public boost::static_visitor<bool> {  
	const BarzelEvalResult& d_childVal;
	const BarzelEvalResult& d_val;

	Eval_visitor_needToStop( BarzelEvalResult& cv, const BarzelEvalResult& v ) : 
		d_childVal(cv),
		d_val(v)
	{}

	/// this may be overridden for control structures such as AND/OR etc. 
	/// it will alter d_val if need be 

	template <typename T>
	bool operator() ( const T& btnd ) 
		{ return false; }
};

template<>
bool Eval_visitor_needToStop::operator()<BTND_Rewrite_Logic>(const BTND_Rewrite_Logic& l)
{
    const BarzelBeadData &childVal = d_childVal.getBeadData();
    switch(l.getType()) {
    case BTND_Rewrite_Logic::AND:
        return !childVal.which();
    case BTND_Rewrite_Logic::OR:
        return childVal.which();
    case BTND_Rewrite_Logic::NOT:
        break;
    }
    return false;
}

inline std::ostream& operator << ( std::ostream& fp, const BarzelEvalResultVec& v ) 
{
    for( BarzelEvalResultVec::const_iterator i = v.begin(); i!= v.end(); ++i ) {
        fp << *i << "\n";
    }
    return fp;
}
inline std::ostream& operator << ( std::ostream& fp, const ay::skippedvector<BarzelEvalResult>& v ) 
{
    return fp << "ay::skipvec " << v.vec() << std::endl;
}

struct Eval_visitor_compute : public boost::static_visitor<bool> {  
	const BarzelEvalResultVec& d_childValVec;
	BarzelEvalResult& d_val;
	BarzelEvalContext &ctxt;
	const BarzelEvalNode &d_evalNode;

	Eval_visitor_compute( 
		const BarzelEvalResultVec& cvv, 
		BarzelEvalResult& v,
		BarzelEvalContext &c, 
		const BarzelEvalNode &n) :
			d_childValVec(cvv),
			d_val(v),
			ctxt(c),
			d_evalNode(n)
	{}


	void returnChildren() {
        BarzelBeadDataVec &valVec = d_val.getBeadDataVec();
        valVec.clear();
        for (BarzelEvalResultVec::const_iterator iter = d_childValVec.begin();
                                                 iter != d_childValVec.end();
                                                 ++iter) {
            const BarzelBeadDataVec &childVec = iter->getBeadDataVec();
            valVec.insert(valVec.end(), childVec.begin(), childVec.end());
        }
	}

	bool operator()( const BTND_Rewrite_None &data ) {
		//AYLOG(DEBUG) << "BTND_Rewrite_None";
	    returnChildren();
		return true;
	}

	/// this should be specialized for various participants in the BTND_RewriteData variant 
	template <typename T> bool operator()( const T& )
	{
		AYLOG(DEBUG) << "unknown";
		return true;
	}


};


template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Literal>(const BTND_Rewrite_Literal &data) {
	//AYLOG(DEBUG) << "calling funid:" << data.nameId;
	//const StoredUniverse &u = ctxt.universe;
	//const BELFunctionStorage &fs = u.getFunctionStorage();
	BarzerLiteral lit( data );

	d_val.setBeadData( 	BarzelBeadAtomic().setData( BarzerLiteral(data) ) );
	return true;
}
template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Control>(const BTND_Rewrite_Control &data) {
    /// evaluating block
    return BarzelControlEval( ctxt, d_childValVec, d_evalNode )( d_val, data );
}

template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Function>(const BTND_Rewrite_Function &data) {
	const BarzelEvalNode* evalNode = ctxt.getTrie().getProcs().getEvalNode( data.nameId );
	if( evalNode ) {
        bool ret = false;
        {
		    BarzelEvalContext::frame_stack_raii frameRaii( ctxt, ay::skippedvector<BarzelEvalResult>(d_childValVec) );
		    ret = evalNode->eval( d_val, ctxt);
        }
        /// if function call successful and it has output variable
        if( ret )  {
            if( data.isValidVar() ) {
                if( data.isRewriteVar() ) { // default output variable is for rewrite
                    ctxt.bindVar(data.getVarId()) = d_val;
                } else if( data.isReqVar() ) {
                    const char* requestVar = ctxt.getUniverse().getGlobalPools().internalString_resolve_safe(data.getVarId());
                    if( const BarzelBeadAtomic* atomic = d_val.getSingleAtomic() )
                        ctxt.getBarz().setReqVarValue(requestVar, atomic->getData() );
                }
             }
        }
		return ret;
	}
	const StoredUniverse &u = ctxt.universe;
	const BELFunctionStorage &fs = u.getFunctionStorage();

	bool ret = fs.call(ctxt, data, d_val, ay::skippedvector<BarzelEvalResult>(d_childValVec), u );
    if( ret && data.isValidVar() ) {
        // ctxt.bindVar(data.getVarId()) = d_val;
        ctxt.bindVar(data.getVarId(), d_val );
    }
	return ret;
}

template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_RuntimeEntlist>( const BTND_Rewrite_RuntimeEntlist& n ) 
{
    d_val.setBeadData( BarzelBeadAtomic().setData( n.lst ) );
    return true;
}
template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_MkEnt>( const BTND_Rewrite_MkEnt& n ) 
{
	if( n.isSingleEnt() ) {
		const StoredEntity * se = ctxt.universe.getDtaIdx().getEntById( n.getEntId() );
		BarzerEntity ent=  se->euid;
		d_val.setBeadData( BarzelBeadAtomic().setData( ent ) );
	} else if( n.isEntList() ) {
		const EntityGroup* entGrp = ctxt.getTrie().getEntGroupById( n.getEntGroupId() );
		if( entGrp ) {
			// building entity list
			BarzerEntityList entList;
			for( EntityGroup::Vec::const_iterator i = entGrp->getVec().begin(); i!= entGrp->getVec().end(); ++i )
			{
				const StoredEntity * se = ctxt.universe.getDtaIdx().getEntById( *i );
				const BarzerEntity& euid = se->getEuid();
				entList.addEntity( euid ); 
			}
			d_val.setBeadData( BarzelBeadAtomic().setData( entList ) );
		}
	}
	return true;
}

template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Number>( const BTND_Rewrite_Number& n ) 
{
	BarzerNumber bNum;
	n.setBarzerNumber( bNum );
	//bNum.print(AYLOG(DEBUG) << "BTND_Rewrite_Number:");
	d_val.setBeadData( BarzelBeadAtomic().setData( bNum ) );
	return true;
}

template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Variable>( const BTND_Rewrite_Variable& n ) 
{
    if( n.isRequestVar() ) {
        const char * vname = ctxt.resolveStringInternal(n.getVarId());
        if( vname ) {
            if( const BarzelBeadAtomic_var* p= ctxt.getBarz().getReqVarValue( vname ) )
                return( d_val.setBeadData( BarzelBeadAtomic().setData(*p) ), true );
            else {
                d_val.setBeadData( BarzelBeadAtomic() );
                return true;
            }
        } 

        return false;
    }
    const BELSingleVarPath* varPath = ( n.isVarId() ? ctxt.matchInfo.getVarPathByVarId(n.getVarId(),ctxt.getTrie()): 0); 
    const BarzelEvalResult* varResult = ( varPath && varPath->size() == 1 ? ctxt.getVar( (*varPath)[0]) :0 );
    if( varResult ) {
        d_val = *varResult;
    } else {
		BarzelMatchInfo& matchInfo = ctxt.matchInfo;
		BeadRange r;
	
		if( matchInfo.getDataByVar(r,n,ctxt.getTrie())  ) {
			if( r.first == r.second )  {
				if( matchInfo.iteratorIsEnd( r.second ) ) {
					std::cerr << "ERROR: blank tail range passed\n";
					return false;
				}
				matchInfo.expandRangeByOne(r);
			}
			//std::cerr << "** COMPUTE *********";
			//AYDEBUG( r );
			BarzelBeadChain::trimBlanksFromRange(r);
			//std::cerr << "** AFTER COMPUTE *********";
			//AYDEBUG( r );
			d_val.setBeadData( r );
		}
	}
    return true;
}


template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Select>
    ( const BTND_Rewrite_Select &s)
{
//    AYLOG(DEBUG) << "BTND_Rewrite_Select";
    BarzelMatchInfo& matchInfo = ctxt.matchInfo;
    BeadRange r;


//    AYLOGDEBUG(s.varId);
    BTND_Rewrite_Variable n;
    n.setVarId(s.varId);

    if( matchInfo.getDataByVar(r, n,ctxt.getTrie())  ) {
        if( r.first == r.second )  {
            if( matchInfo.iteratorIsEnd( r.second ) ) {
                // std::cerr << "ERROR: blank tail range passed\n";
                ctxt.pushBarzelError( "<e>non literal passed into case</e>" ); 
                return false;
            }
            matchInfo.expandRangeByOne(r);
        }

        BarzelBeadChain::trimBlanksFromRange(r);


        const BarzelBeadAtomic *a = boost::get<BarzelBeadAtomic>(&(r.first->getBeadData()));
        const BarzerLiteral *ltrlPtr = ( a ? boost::get<BarzerLiteral>(&(a->getData())) : 0 );
        BarzerLiteral tmpLiteral;
        if( !ltrlPtr ) {
            const BarzerString* s = ( a ? boost::get<BarzerString>(&(a->getData())) : 0 );
            if( s && s->getStemStringId() != 0xffffffff) {
                tmpLiteral = BarzerLiteral( s->getStemStringId() );
                ltrlPtr = &tmpLiteral;
            } else {
                ctxt.pushBarzelError( "<e>non literal passed into case</e>" ); 
                return false;
            }
        }
        const BarzerLiteral& ltrl = *ltrlPtr;

        const BarzelEvalNode::ChildVec &child = d_evalNode.getChild();

        CaseFinder cf(ltrl.getId());

        BarzelEvalNode::ChildVec::const_iterator it =
                std::lower_bound(child.begin(), child.end(), 0, cf);
        if (it != child.end()) {
            return it->eval(d_val, ctxt );
        }
    }
    return false;
}

template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Case>
    ( const BTND_Rewrite_Case &s)
{

    returnChildren();
    return true;
}


template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Logic>
    ( const BTND_Rewrite_Logic &s)
{
    switch(s.getType()) {
    case BTND_Rewrite_Logic::AND:
    case BTND_Rewrite_Logic::OR:
        if( d_childValVec.size() ) {
            d_val.setBeadData(d_childValVec.back().getBeadData());
        } else
            return true;
        break;
    case BTND_Rewrite_Logic::NOT: {
        BarzelBeadDataVec &valVec =  d_val.getBeadDataVec();
        valVec.clear();
        valVec.reserve(d_childValVec.size());
        for (BarzelEvalResultVec::const_iterator it = d_childValVec.begin(),
                                                      end = d_childValVec.end();
                                                 it != end; ++it) {
            if (it->getBeadData().which()) {
                valVec.push_back(BarzelBeadBlank());
            } else {
                valVec.push_back(BarzelBeadAtomic());
            }
        }
    }
    }

    return true;
}


//// the main visitor - compute

} // end of anon namespace 


bool BarzelEvalNode::isFallible() const 
{
	if( ( d_btnd.which() == BTND_Rewrite_EntitySearch_TYPE ) ) 
		return true;
	for( ChildVec::const_iterator i = d_child.begin(); i!= d_child.end(); ++i ) {
		if( i->isFallible() ) 
			return true;
	}
	return false;
}

bool BarzelEvalNode::eval_comma(BarzelEvalResult& val, BarzelEvalContext&  ctxt ) const
{
    if( !d_child.size() ) 
        return false;

    // const BarzelEvalProcFrame* topFrame = ctxt.getTopProcFrame();

	if( d_child.size()> 1 ) {
		
		/// forming dependent vector of values 
        size_t childSize = d_child.size()-1;
		for( size_t i =0; i< childSize; ++i ) {
			// const BarzelEvalNode& childNode = d_child[i];

            //// if childNode is a let node - assign variable and continue

            BarzelEvalResult childVal;

			size_t substPos= 0xffffffff;
			if( d_child[i].isSubstitutionParm( substPos ) ) {
			} else {
				if( !d_child[i].eval(childVal, ctxt) )
					return false; // error in one of the children occured
	
            	Eval_visitor_needToStop visitor(childVal,val);
            	if( boost::apply_visitor( visitor, d_btnd ) )
                	break;
			}
		}
		/// vector of dependent values ready
	}
    bool ret = d_child.back().eval(val, ctxt) ;

	return ret;
}

bool BarzelEvalNode::eval(BarzelEvalResult& val, BarzelEvalContext&  ctxt ) const
{
    const BTND_Rewrite_Control* ctrl = isComma();
    if( ctrl ) { /// this is a block (comma)
		// BarzelEvalContext::frame_stack_raii frameRaii( ctxt, ay::skippedvector<BarzelEvalResult>(d_childValVec) );
        if( eval_comma( val, ctxt ) ) {
            if(ctrl->isValidVar()) { // if block has a variable we bind it 
                if( ctrl->isRewriteVar() ) { // default output variable is for rewrite
                    ctxt.bindVar(ctrl->getVarId()) = val;
                } else if( ctrl->isReqVar() ) {
                    const char* requestVar = 
                        ctxt.getUniverse().getGlobalPools().internalString_resolve_safe(ctrl->getVarId());
                    if( const BarzelBeadAtomic* atomic = val.getSingleAtomic() )
                        ctxt.getBarz().setReqVarValue(requestVar, atomic->getData() );
                }
            } 
            return true;
        } else
            return false;
    }

	BarzelEvalResultVec childValVec;

    // const BarzelEvalProcFrame* topFrame = ctxt.getTopProcFrame();

	if( boost::apply_visitor(Eval_visitor_evalChildren(), d_btnd)
	        && d_child.size() ) {
		
	    childValVec.reserve( d_child.size() );

		/// forming dependent vector of values 
		for( size_t i =0; i< d_child.size(); ++i ) {
			/// const BarzelEvalNode& childNode = d_child[i];

            //// if childNode is a let node - assign variable and continue

            childValVec.resize(i+1);
            // BarzelEvalResult& childVal = childValVec.back();

			size_t substPos= 0xffffffff;
			if( d_child[i].isSubstitutionParm( substPos ) ) {
				const BarzelEvalResult* subsResult = ctxt.getPositionalArg( substPos );
				if( subsResult ) {
					childValVec.back().setBeadData(subsResult->getBeadData());
                }
			} else {
				if( !d_child[i].eval(childValVec.back(), ctxt) )
					return false; // error in one of the children occured
	
            	Eval_visitor_needToStop visitor(childValVec.back(),val);
            	if( boost::apply_visitor( visitor, d_btnd ) )
                	break;
			}
		}
		/// vector of dependent values ready
	}

	Eval_visitor_compute visitor(childValVec,val,ctxt, *this);
	bool ret = boost::apply_visitor( visitor, d_btnd );
	
	return ret;

}

const uint8_t* BarzelEvalNode::growTree_recursive( BarzelEvalNode::ByteRange& brng, int& ctxtErr )
{
	const uint8_t* buf = brng.first;
	const uint16_t childStep_sz = 1 + sizeof(BTND_RewriteData);
	for( ; buf < brng.second; ++buf) {

		switch( *buf ) {
		case barzel::RWR_NODE_START: {
			if( buf + childStep_sz >= brng.second ) 
				return ( ctxtErr = BarzelEvalContext::EVALERR_GROW, (const uint8_t*)0 );

			BTND_RewriteData *rdp = (BTND_RewriteData*)(buf+1);
			d_child.push_back(*rdp);

			BarzelEvalNode::ByteRange childRange( (buf + childStep_sz ), brng.second);
			buf = d_child.back().growTree_recursive( childRange, ctxtErr );
			if( !buf ) 
				return ( ctxtErr = BarzelEvalContext::EVALERR_GROW, (const uint8_t*)0 );
		}
			break;
		case barzel::RWR_NODE_END:
			return ( ctxtErr = BarzelEvalContext::EVALERR_OK, buf );
		default:
			return ( ctxtErr = BarzelEvalContext::EVALERR_GROW, (const uint8_t*)0 );
		}
	}
	return ( ctxtErr = BarzelEvalContext::EVALERR_OK, buf ); 
}

namespace {

struct BTND_RewriteData_printer : public boost::static_visitor<> {
	std::ostream& fp;
	const BELPrintContext& ctxt;
	BTND_RewriteData_printer( std::ostream& f, const BELPrintContext& c ) : fp(f), ctxt(c) {} 

	template <typename T> void operator()( const T& t ) const { t.print( fp, ctxt ); }
};

template <> void BTND_RewriteData_printer::operator()<BTND_Rewrite_RuntimeEntlist>( const BTND_Rewrite_RuntimeEntlist& t ) const 
{ fp << "runtime ent list"; }
template <> void BTND_RewriteData_printer::operator()<BTND_Rewrite_DateTime>( const BTND_Rewrite_DateTime& t ) const 
{ fp << "DateTime"; }
template <> void BTND_RewriteData_printer::operator()<BTND_Rewrite_EntitySearch>( const BTND_Rewrite_EntitySearch& t ) const 
{ fp << "EntSearch"; }
} // anon namespace 

	bool BRBCPrintCB::nodeData( const BTND_RewriteData& d )
	{
		BTND_RewriteData_printer printer(fp, ctxt);
		boost::apply_visitor( printer, d );
		return true;
	}

	bool BRBCPrintCB::nodeStart() 
	{
		fp << prefix << "(\n"; 
		prefix.append( 4, ' ' );
		return true;
	}
	bool BRBCPrintCB::nodeEnd() 
	{
		fp << prefix << ")\n"; 
		if( prefix.length() >= 4 )  prefix.resize(prefix.size()-4); 
		return true; 
	}


const RequestEnvironment* BarzelEvalContext::getRequestEnvironment() { return d_barz.getServerReqEnv(); }
BarzelEvalContext& BarzelEvalContext::pushBarzelError( const char* err )
{
    d_barz.barzelTrace.pushError( err );
    return *this;
}

const BarzerDateTime* BarzelEvalContext::getNowPtr( ) const
{
    if( const RequestEnvironment* reqEnv = d_barz.getServerReqEnv() )
        return reqEnv->getNowPtr() ;
    return 0;
}
const char* BarzelEvalContext::resolveStringInternal( uint32_t i ) const
{
    return universe.getGlobalPools().internalString_resolve(i);
}

} /// barzer namespace ends
