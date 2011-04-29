#include <barzer_el_rewriter.h>
#include <barzer_el_function.h>
#include <barzer_universe.h>
#include <ay/ay_logger.h>
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

} // anon namespace ends 

bool  BarzelRewriterPool::isRewriteFallible( const BarzelRewriterPool::BufAndSize& bas ) const 
{
	Fallible fallible;
	BarzelRewriteByteCodeProcessor<Fallible> processor(fallible,bas);
	return fallible.isThatSo;
}

int  BarzelRewriterPool::encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn ) const
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

	for( BELParseTreeNode::ChildrenVec::const_iterator ch = ptn.child.begin(); ch != ptn.child.end(); ++ch ) {
		
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

/// this visitor would only make sense for operators that may need less than 
/// all of their dependent values computd. this primarily relates to 
/// control structures 
struct Eval_visitor_needToStop : public boost::static_visitor<bool> {  
	const BarzelEvalResult& d_childVal;
	BarzelEvalResult& d_val;

	Eval_visitor_needToStop( BarzelEvalResult& cv, BarzelEvalResult& v ) : 
		d_childVal(cv),
		d_val(v)
	{}
	/// this may be overridden for control structures such as AND/OR etc. 
	/// it will alter d_val if need be 
	template <typename T>
	bool operator() ( const T& btnd ) 
		{ return false; }
};
struct Eval_visitor_compute : public boost::static_visitor<bool> {  
	const BarzelEvalResultVec& d_childValVec;
	BarzelEvalResult& d_val;
	BarzelEvalContext &ctxt;

	Eval_visitor_compute( const BarzelEvalResultVec& cvv, BarzelEvalResult& v,
						  BarzelEvalContext &c) :
		d_childValVec(cvv),
		d_val(v),
		ctxt(c)
	{}


	/// this should be specialized for various participants in the BTND_RewriteData variant 
	template <typename T> bool operator()( const T& )
	{
		return true;
	}


};


template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Function>(const BTND_Rewrite_Function &data) {
	AYLOG(DEBUG) << "calling funid:" << data.nameId;
	const StoredUniverse &u = ctxt.universe;
	const BELFunctionStorage &fs = u.getFunctionStorage();
	return fs.call(data.nameId, d_val, d_childValVec);
}

template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Number>( const BTND_Rewrite_Number& n ) 
{
	BarzerNumber bNum;
	n.setBarzerNumber( bNum );
	d_val.setBeadData( BarzelBeadAtomic().setData( bNum ) );
	return true;
}
template <> bool Eval_visitor_compute::operator()<BTND_Rewrite_Variable>( const BTND_Rewrite_Variable& n ) 
{
	return true;
}

//// the main visitor - compute

} // end of anon namespace 

bool BarzelEvalNode::eval(BarzelEvalResult& val, BarzelEvalContext&  ctxt ) const
{
	BarzelEvalResultVec childValVec;
	if( d_child.size() ) {
		childValVec.resize( d_child.size() );
		
		/// forming dependent vector of values 
		for( size_t i =0; i< d_child.size(); ++i ) {
			const BarzelEvalNode& childNode = d_child[i];
			BarzelEvalResult& childVal = childValVec[i];
			if( !childNode.eval(childVal, ctxt) ) 
				return false; // error in one of the children occured
			
			Eval_visitor_needToStop visitor(childVal,val);
			if( boost::apply_visitor( visitor, d_btnd ) ) 
				return false;
		}
		/// vector of dependent values ready
	}

	Eval_visitor_compute visitor(childValVec,val,ctxt);

	return boost::apply_visitor( visitor, d_btnd );
}

const uint8_t* BarzelEvalNode::growTree_recursive( BarzelEvalNode::ByteRange& brng, BarzelEvalContext& ctxt )
{
	//AYLOG(DEBUG) << "growTree_recursive called";
	const uint8_t* buf = brng.first;
	const uint16_t childStep_sz = 1 + sizeof(BTND_RewriteData);
	//uint8_t tmp[ sizeof(BTND_RewriteData) ];
	for( ; buf < brng.second; ++buf) {

		switch( *buf ) {
		case barzel::RWR_NODE_START: {
			if( buf + childStep_sz >= brng.second ) 
				return ctxt.setErr_GROW();

			//memcpy( tmp, buf+1, sizeof(tmp) );
			//BTND_RewriteData *rdp = (BTND_RewriteData*) tmp;
			//AYLOG(DEBUG) << rdp->which();
			d_child.push_back(*(BTND_RewriteData*)(buf+1));
			//d_child.push_back( *(new(tmp) BTND_RewriteData()) );

			BarzelEvalNode::ByteRange childRange( (buf + childStep_sz ), brng.second);
			buf = d_child.back().growTree_recursive( childRange, ctxt );
			if( !buf ) 
				return ctxt.setErr_GROW();
		}
			break;
		case barzel::RWR_NODE_END:
			return buf;
		default:
			return ctxt.setErr_GROW();
		}
	}
	return 0;
}

namespace {

struct BTND_RewriteData_printer : public boost::static_visitor<> {
	std::ostream& fp;
	const BELPrintContext& ctxt;
	BTND_RewriteData_printer( std::ostream& f, const BELPrintContext& c ) : fp(f), ctxt(c) {} 

	template <typename T> void operator()( const T& t ) const { t.print( fp, ctxt ); }
};

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
} /// barzer namespace ends
