#include <barzer_el_rewriter.h>

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

namespace barzel {
	enum {
		RWR_NODE_START = 1,
		RWR_NODE_END
	};
}

bool  BarzelRewriterPool::isRewriteFallible( const BarzelRewriterPool::BufAndSize& bas ) const 
{
	const uint8_t * buf = bas.first;
	const uint8_t * buf_end = bas.first + bas.second;
	
	/// linear scan of all nodes - no need to have visitor - for now only EntitySearch is considered fallible
	/// heoretically there can be infallible entity search patterns and if and when it becomes relevant 
	/// i will update the logic - it would require analysing the subtree (AY) 
	for( const uint8_t * b = buf+1; b< buf_end; b+= ( 2 + sizeof(BTND_RewriteData) ) ) {
		const BTND_Rewrite_EntitySearch* srch = boost::get<BTND_Rewrite_EntitySearch>( (new(b) BTND_RewriteData()) );
		if( srch ) 
			return true;
	}
	return true;
}

int  BarzelRewriterPool::encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn ) const
{
	// for every node push node_start_byte, then BTND_Rewrite (the whole variant) (otn.getNodeData() ) - memcpy, then node end byte

	trans.push_back( (uint8_t)barzel::RWR_NODE_START );
	const BTND_RewriteData* rd = ptn.getRewriteData();
	if( !rd ) 
		return ERR_ENCODING_FAILED;

	const uint8_t* rdBuf = (const uint8_t*) rd;
	trans.insert( trans.end(), rdBuf, rdBuf+sizeof(BTND_RewriteData) );

	for( BELParseTreeNode::ChildrenVec::const_iterator ch = ptn.child.begin(); ch != ptn.child.end(); ++ch ) {
		
		int rc =encodeParseTreeNode( trans, *ch );
		if( rc ) 
			return rc; // error
	}
	trans.push_back( (uint8_t)barzel::RWR_NODE_END );
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
	Eval_visitor_compute( 	const BarzelEvalResultVec& cvv; BarzelEvalResult& v ) : 
		d_childValVec(cvv),
		d_val(v)
	{}

	/// this should be specialized for various participants in the BTND_RewriteData variant 
	template <typename T>
	bool operator()( const T& ) 
	{
		return true;
	}
};

//// the main visitor - compute

} // end of anon namespace 

bool BarzelEvalNode::eval(BarzelEvalResult& val, BarzelEvalNode::EvalContext& ctxt ) const
{
	if( d_child.size() ) {
		BarzelEvalResultVec childValVec;
		childValVec.resize( d_child.size() );
		
		/// forming dependent vector of values 
		for( size_t i =0; i< d_child.size(); ++i ) {
			const BarzelEvalNode& cNode = d_child[i];
			BarzelEvalResult& childVal = childValVec[i];
			if( !childNode.eval(childVal, ctxt) ) 
				return false; // error in one of the children occured
			
			if( boost::apply_visitor( Eval_visitor_needToStop(childVal,val), d_btnd ) ) 
				return 0;
		}
		/// vector of dependent values ready
		return boost::apply_visitor( Eval_visitor_compute(childValVec,val), d_btnd );
	}
}

const uint8_t* BarzelEvalNode::growTree_recursive( BarzelEvalNode::ByteRange& brng, EvalContext& ctxt )
{
	const uint8_t* buf = bas.first;
	const uint16_t childStep_sz = 1 + sizeof(BTND_RewriteData);
	for( ; buf < brng.second; ++buf ) {

		switch( *buf ) {
		case barzel::RWR_NODE_START: {
			if( buf + childStep_sz >= brng.second ) 
				return ctxt.setErr_GROW();

			d_child.push_back( *(new(buf+1) BTND_RewriteData()) ); 

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
} /// barzer namespace ends
