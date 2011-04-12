#include <barzer_el_rewriter.h>

namespace barzer {

BarzelRewriterPool::~BarzelRewriterPool()
{
	for( BufAndSizeVec::iterator i = encVec.begin(); i!= encVec.end(); ++i ) 
		free((uint8_t*)(i->first));
}

namespace barzel {
	enum {
		RWR_NODE_START = 1,
		RWR_NODE_END
	};
}

int  BarzelRewriterPool::encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn )
{
	trans.push_back( (uint8_t)barzel::RWR_NODE_START );
	const BTND_RewriteData* rd = ptn.getRewriteData();
	if( !rd ) 
		return ERR_ENCODING_FAILED;

	/// produce header

	int rwrTypeId = rd->which();
	uint8_t header[ 2 ] = {0};
	header[0] = (uint8_t)rwrTypeId;
	switch( rwrTypeId ) {
	case BTND_Rewrite_DateTime_TYPE:
		header[1] = boost::get< BTND_Rewrite_DateTime >( ptn.getNodeData() ).which();
		break;
	case BTND_Rewrite_Range_TYPE:
		header[1] = boost::get< BTND_Rewrite_Range >( ptn.getNodeData() ).dta.which();
		break;
	default:
		break;
	}
	trans.push_back( header[0] );
	trans.push_back( header[1] );
	// end of header production

	// processing children
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

}
