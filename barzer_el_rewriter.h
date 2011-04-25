#ifndef BARZER_EL_REWRITER_H
#define BARZER_EL_REWRITER_H
#include <barzer_el_btnd.h>
#include <barzer_el_parser.h>
#include <barzer_el_chain.h>
namespace barzer {
/// rewriter is a tree which is evaluated based on input
/// result of evaluation is a value computed from underlying nodes
/// value is a constant and can be of one of the constant types 
/// string, compounded word, number, date, time, entity
/// range - range has range type and the actual to-from range 
///  - range type is either a hardcoded thing or an entity 
/// 
/// input is RewriterContext which has a hash table with variables and other 
/// stuff

class BarzelTranslation;

/// unique at least per BarzelTrie (most likely globally unique)
/// the pool stores binary buffers with barzel rewrite instructions
/// these buffers are used to build BarzelRewriteTree objects on the fly 
class BarzelRewriterPool {
public:
	typedef std::pair<const uint8_t*, uint32_t> BufAndSize;
private:
	typedef std::vector<uint8_t> byte_vec;
	typedef std::vector< BufAndSize > BufAndSizeVec;
	BufAndSizeVec encVec;

	uint32_t poolNewBuf( const uint8_t* s, uint32_t sz )
	{
		encVec.push_back( BufAndSize( (uint8_t*)malloc(sz), sz) );
		return (encVec.size() -1);
	}

	int  encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn ) const;
public:
	void clear();
	~BarzelRewriterPool();
	BarzelRewriterPool( size_t reserveSz ) 
		{ encVec.reserve( reserveSz ) ; }


	enum {
		ERR_OK,
		ERR_ENCODING_FAILED, // input tree is invalid
		ERR_ENCODING_INCONSISTENT // some internal inconsistency encountered
	};
	// returns one of the ERR_XXX enums
	int produceTranslation( BarzelTranslation& , const BELParseTreeNode& ptn );

	/// when translation contained valid rewriter buffer returns true
	/// otherwise 0
	bool resolveTranslation( BufAndSize&, const BarzelTranslation& trans ) const;
	/// returns true if any EntitySearch rewrites are encountered anywhere 
	/// in the encoding - this performs a linear scan 
	/// of the rewrite bytecode . even if this function returns true the rewrite 
	/// may still never fail. this is a quick way to check though during matching 
	bool isRewriteFallible( const BufAndSize& bas ) const;
};

//// rewriter evaluation 
class StoredUniverse;
class BarzelMatchInfo;

struct BarzelEvalResult {
	BarzelBeadData d_val;	
	const BarzelBeadData& getBeadData() const { return d_val; }
};
typedef std::vector< BarzelEvalResult > BarzelEvalResultVec;

struct BarzelEvalContext {
	BarzelMatchInfo& matchInfo;
	const StoredUniverse& universe;
	int err;
	enum {
		EVALERR_OK, 
		EVALERR_GROW
	};

	bool hasError() { return err != EVALERR_OK; }
	const uint8_t* setErr_GROW() { return( err = EVALERR_GROW, (const uint8_t*)0); } 
	BarzelEvalContext( BarzelMatchInfo& mi, const StoredUniverse& uni ) : 
		matchInfo(mi), universe(uni) , err(EVALERR_OK)
	{}
};
class BarzelEvalNode {
protected:
	BTND_RewriteData d_btnd;

	typedef std::vector< BarzelEvalNode > ChildVec;
	ChildVec d_child;


	//// will recursively add nodes 
	typedef std::pair< const uint8_t*, const uint8_t* > ByteRange;
	const uint8_t* growTree_recursive( ByteRange& brng, BarzelEvalContext& ctxt );
	
public:
	BTND_RewriteData& getBtnd() { return d_btnd; }

	const BTND_RewriteData& getBtnd() const { return d_btnd; }
	bool isFallible() const 
	{
		if( ( d_btnd.which() == BTND_Rewrite_EntitySearch_TYPE ) ) 
			return true;
		for( ChildVec::const_iterator i = d_child.begin(); i!= d_child.end(); ++i ) {
			if( i->isFallible() ) 
				return true;
		}
		return false;
	}
	BarzelEvalNode() {}
	BarzelEvalNode( const BTND_RewriteData& b ) : d_btnd(b) {}

	/// construct the tree from the byte buffer created in encode ... 
	/// returns true is tree was constructed successfully
	bool growTree( const BarzelRewriterPool::BufAndSize& bas, BarzelEvalContext& ctxt )
	{
		ByteRange brng( bas.first, bas.first+ bas.second );
		return( growTree_recursive( brng, ctxt ) !=0 ) ;
	}
	/// returns true if evaluation is successful 
	bool eval(BarzelEvalResult&, BarzelEvalContext& ctxt ) const;
};

}

#endif // BARZER_EL_REWRITER_H
