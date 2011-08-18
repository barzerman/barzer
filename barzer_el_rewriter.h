#ifndef BARZER_EL_REWRITER_H
#define BARZER_EL_REWRITER_H
#include <stack>
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
	typedef std::vector<uint8_t> byte_vec;
	typedef std::vector< BufAndSize > BufAndSizeVec;
	
	inline static void byte_vec2bas( BufAndSize& bas, byte_vec& bv ) 
	{
		bas.first = &(bv[0]);
		bas.second = bv.size();
	}
private:
	BufAndSizeVec encVec;

	uint32_t poolNewBuf( const uint8_t* s, uint32_t sz )
	{
		uint8_t* buf = (uint8_t*)malloc(sz);
		memcpy( buf, s,sz);
		encVec.push_back( BufAndSize( buf, sz) );
		return (encVec.size() -1);
	}

public:
	static int  encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn );
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


namespace barzel {
	enum {
		RWR_NODE_START = 1,
		RWR_NODE_END
	};
}


//// rewriter evaluation 
class StoredUniverse;
class BarzelMatchInfo;

typedef std::vector< BarzelBeadData > BarzelBeadDataVec;
class BarzelEvalResult {
public:
	typedef std::vector< BarzelBeadData > BarzelBeadDataVec;
	typedef std::vector< CTWPVec > CTokVecVec;
private:
	/// this vector can never be shorter than 1 element 
	BarzelBeadDataVec d_val;	
	CTokVecVec        d_origToks;
public:
	BarzelEvalResult() : d_val(1) {}
	const BarzelBeadData& getBeadData() const { return d_val[0]; }
	      BarzelBeadData& getBeadData()       { return d_val[0]; }

	const BarzelBeadDataVec& getBeadDataVec() const { return d_val; }
	      BarzelBeadDataVec& getBeadDataVec()       { return d_val; }
	bool isVec() const { return (d_val.size() > 1); }
	template <typename T> void setBeadData( const T& t ) { d_val[0] = t; }
	/*
	const CTokVecVec& getOrigToks() const
		{ return d_origToks; }
	void addOrigTokens( const CTWPVec& v ) 
	{
		if( !d_origToks.size() ) d_origToks.resize(1);
		d_origToks.back().insert( d_origToks.back().end(), v.begin(), v.end() ) ;
	}
	*/
	//template <typename T> T& setBeadData( const T& t )
//		{ return (d_val[0] = t); }
};

template <>
inline void BarzelEvalResult::setBeadData<BarzelBeadDataVec>( const BarzelBeadDataVec& v )
{
	d_val.clear();
	d_val.insert(d_val.end(), v.begin(), v.end());
	if (!d_val.size())
		d_val.resize(1);
}


template <>
inline void BarzelEvalResult::setBeadData<BarzelBeadRange>( const BarzelBeadRange& t ) 
{
	d_val.clear();
	for( BeadList::iterator i = t.first; i!= t.second; ++i )  {
		d_val.push_back( i->getBeadData() );
		// d_origToks.push_back( i->getCTokens() );
	}

	if( !d_val.size() ) {
		d_val.resize(1);
		// d_origToks.resize(1);
	}
}
 //*/

typedef std::vector< BarzelEvalResult > BarzelEvalResultVec;

struct BarzelEvalProcFrame {
	BarzelEvalResultVec resVec;
	
	BarzelEvalProcFrame( ) {}
	BarzelEvalProcFrame( const BarzelEvalResultVec& rv ) : resVec( rv ) {}
	const BarzelEvalResult* getPositionalArg( size_t pos ) const
		{ return( pos< resVec.size() ? &(resVec[pos]) : 0 ); }
};

struct BarzelEvalContext {
	BarzelMatchInfo& matchInfo;
	const StoredUniverse& universe;
	const BELTrie& d_trie;
	int err;
	enum {
		EVALERR_OK, 
		EVALERR_GROW
	};
	
	std::stack< BarzelEvalProcFrame > d_procFrameStack; 
	struct frame_stack_raii {
		BarzelEvalContext& context;
		frame_stack_raii( BarzelEvalContext& ctxt, const BarzelEvalResultVec& rv ) : 
			context(ctxt)
		{ 
			context.d_procFrameStack.push( BarzelEvalProcFrame() );
			context.d_procFrameStack.top().resVec = rv;
		}

		frame_stack_raii( BarzelEvalContext& ctxt, const BarzelEvalProcFrame& frame ):
			context(ctxt)
		{
			context.d_procFrameStack.push( frame );
		}
		~frame_stack_raii() { context.d_procFrameStack.pop(); }
	};
	const BarzelEvalResult* getPositionalArg( size_t pos ) const
		{ return ( d_procFrameStack.empty() ? 0 : d_procFrameStack.top().getPositionalArg(pos) ); }

	bool hasError() { return err != EVALERR_OK; }
	const uint8_t* setErr_GROW() { return( err = EVALERR_GROW, (const uint8_t*)0); } 
	const BELTrie& getTrie() const { return d_trie; }

	BarzelEvalContext( BarzelMatchInfo& mi, const StoredUniverse& uni, const BELTrie& trie ) : 
		matchInfo(mi), universe(uni) , d_trie(trie),err(EVALERR_OK)
	{}
};

class BarzelEvalNode {
public:
    typedef std::vector< BarzelEvalNode > ChildVec;
protected:
	BTND_RewriteData d_btnd;

	ChildVec d_child;


	//// will recursively add nodes 
public:
	typedef std::pair< const uint8_t*, const uint8_t* > ByteRange;
private:
	const uint8_t* growTree_recursive( ByteRange& brng, int& ctxtErr );
	
	bool isSubstitutionParm( size_t& pos ) const
	{
		if( d_btnd.which()  == BTND_Rewrite_Variable_TYPE ) {
			const BTND_Rewrite_Variable& var = boost::get< BTND_Rewrite_Variable >( d_btnd );
			return ( var.isPosArg() ? (pos = var.getVarId(), true) : false );
		} else 
			return ( false );
	}
public:
	BTND_RewriteData& getBtnd() { return d_btnd; }
	ChildVec& getChild() { return d_child; }
	const ChildVec& getChild() const { return d_child; }

	const BTND_RewriteData& getBtnd() const { return d_btnd; }
	bool isFallible() const ;
	BarzelEvalNode() {}
	BarzelEvalNode( const BTND_RewriteData& b ) : d_btnd(b) {}

	/// construct the tree from the byte buffer created in encode ... 
	/// returns true is tree was constructed successfully
	bool growTree( const BarzelRewriterPool::BufAndSize& bas, int& ctxtErr )
	{
		ByteRange brng( bas.first, bas.first+ bas.second );
		return( growTree_recursive( brng, ctxtErr ) !=0 ) ;
	}
	/// returns true if evaluation is successful 
	bool eval(BarzelEvalResult&, BarzelEvalContext& ctxt ) const;
};
//// T must have methods:
////   bool T::nodeStart()
////   bool T::nodeEnd()
////   bool T::nodeData( const BTND_RewriteData& ) 
////   false returned from any of these functions aborts processing

template <typename T>
struct BarzelRewriteByteCodeProcessor {
	
	BarzelEvalNode::ByteRange d_rng;
	T& d_cb;

	BarzelRewriteByteCodeProcessor( T& cb, const BarzelRewriterPool::BufAndSize& bas ) :
		d_rng(bas.first, bas.first+bas.second),
		d_cb(cb)
	{}
	BarzelRewriteByteCodeProcessor( T& cb, const BarzelEvalNode::ByteRange r ) :
		d_rng(r),
		d_cb(cb)
	{}
	
	const uint8_t* run( ){
		const uint8_t* buf = d_rng.first;
		const uint16_t childStep_sz = 1 + sizeof(BTND_RewriteData);
		//uint8_t tmp[ sizeof(BTND_RewriteData) ];
		for( ; buf < d_rng.second; ++buf ) {
	
			switch( *buf ) {
			case barzel::RWR_NODE_START: {
				if( buf + childStep_sz >= d_rng.second ) 
					return 0;
				if( !d_cb.nodeStart() )
					return 0;

				//memcpy( tmp, buf+1, sizeof(tmp) );
				//if( !d_cb.nodeData( *(new(tmp) BTND_RewriteData()) ) )
					//return 0;
				if( !d_cb.nodeData( *(BTND_RewriteData*)(buf+1) )) return 0;
	
				BarzelEvalNode::ByteRange childRange( (buf + childStep_sz ), d_rng.second);
				BarzelRewriteByteCodeProcessor proc( d_cb, childRange );
				buf = proc.run();
				if( !buf ) 
					return 0;
			}
				break;
			case barzel::RWR_NODE_END:
				return( d_cb.nodeEnd() ? buf : 0 ) ;
			default:
				return 0;
			}
		}
		return 0;
	}
};
struct BRBCPrintCB {
	mutable std::string   prefix;
	std::ostream& fp;
	const BELPrintContext& ctxt;

	BRBCPrintCB( std::ostream& f, const BELPrintContext& c ) : fp(f),ctxt(c) {}
	bool nodeStart();
	bool nodeEnd();
	bool nodeData( const BTND_RewriteData& d );
};
	
} // barzer namespace 

#endif // BARZER_EL_REWRITER_H
