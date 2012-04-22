#ifndef BARZER_EL_REWRITER_H
#define BARZER_EL_REWRITER_H
#include <stack>
//#include <barzer_emitter.h>
#include <barzer_el_parser.h>
#include <barzer_el_chain.h>
#include <ay/ay_vector.h>
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

    int d_unmatchable;
public:

	BarzelEvalResult() : d_val(1), d_unmatchable(0) {}

    int getUnmatchability() const { return d_unmatchable; }
    
    void setUnmatchability(int mb=1) { d_unmatchable= mb; }

	const BarzelBeadData& getBeadData() const { return d_val[0]; }
	      BarzelBeadData& getBeadData()       { return d_val[0]; }

    const BarzelBeadAtomic* getSingleAtomic() const { 
        if( d_val[0].which() == BarzelBeadAtomic_TYPE) { 
            return boost::get<BarzelBeadAtomic>( &(d_val[0]) );
        }  else
            return 0;
    }

	const BarzelBeadDataVec& getBeadDataVec() const { return d_val; }
	      BarzelBeadDataVec& getBeadDataVec()       { return d_val; }
	bool isVec() const { return (d_val.size() > 1); }
	template <typename T> void setBeadData( const T& t ) { d_val[0] = t; }
    

    std::ostream& print( std::ostream& fp ) const
    {
        return ( fp << getBeadData() );
    }
};
inline std::ostream& operator<< ( std::ostream& fp, const BarzelEvalResult& x )
{
    return x.print(fp);
}
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

typedef boost::unordered_map< uint32_t, BarzelEvalResult > VariableEvalMap;

class BarzelEvalProcFrame {
    BarzelEvalResultVec d_v;
    size_t d_skip;
    VariableEvalMap d_varMap;	

    const BarzelEvalResult* getLocalVar( uint32_t v ) const 
    {
        VariableEvalMap::const_iterator i = d_varMap.find(v);
        return ( i == d_varMap.end() ? 0 : &(i->second) );
    }
public:
	BarzelEvalProcFrame( const BarzelEvalProcFrame& x )  : 
        d_v(x.d_v), d_skip(x.d_skip)
    {}

    
	BarzelEvalProcFrame( )  : d_skip(0) {}
	BarzelEvalProcFrame( const ay::skippedvector<BarzelEvalResult>& rv ) : d_v(rv.vec()), d_skip(rv.getSkip() ) {}

	const BarzelEvalResult* getPositionalArg( size_t pos ) const
		{ return( (pos+ d_skip)< d_v.size() ? &(d_v[d_skip+pos]) : 0 ); }

    void assign( const ay::skippedvector<BarzelEvalResult>& rv )
    {
        d_v = rv.vec();
        d_skip = rv.getSkip();
    }
    
    /// adds to the current frame - this fucntion should always return non 0
    BarzelEvalResult& bind( uint32_t varId )
    {
        VariableEvalMap::iterator i = d_varMap.insert( VariableEvalMap::value_type(varId, BarzelEvalResult()) ).first;
        return (i->second);
    }
    BarzelEvalResult* bindPtr( uint32_t varId )
    {
        VariableEvalMap::iterator i = d_varMap.insert( VariableEvalMap::value_type(varId, BarzelEvalResult()) ).first;
        return &(i->second);
    }
    
    const BarzelEvalResult* getVar( uint32_t varId ) const 
        { return getLocalVar( varId ); }
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
    Barz& d_barz;
	
	typedef  std::vector< BarzelEvalProcFrame > BarzelEvalProcFrameStack;
	BarzelEvalProcFrameStack d_procFrameStack; 
	struct frame_stack_raii {
		BarzelEvalContext& context;
		frame_stack_raii( BarzelEvalContext& ctxt, const ay::skippedvector<BarzelEvalResult>& rv ) : 
			context(ctxt)
		{ 
			context.d_procFrameStack.push_back( BarzelEvalProcFrame() );
			context.d_procFrameStack.back().assign(rv);
		}

		frame_stack_raii( BarzelEvalContext& ctxt, const BarzelEvalProcFrame& frame ):
			context(ctxt)
		{
			context.d_procFrameStack.push_back( frame );
		}
		~frame_stack_raii() { 
            if( context.d_procFrameStack.size() ) 
                context.d_procFrameStack.resize( context.d_procFrameStack.size()-1 );
        }
	};
    // let var 
    BarzelEvalResult& bindVar( uint32_t varId ) 
        { return d_procFrameStack.back().bind(varId); }
    /// binds 
    BarzelEvalResult* bindVarUpTheStack( uint32_t varId ) 
        {  
            if( d_procFrameStack.size()>1 ) 
                return (d_procFrameStack[ d_procFrameStack.size() -2 ].bindPtr(varId));
            else
                return 0;
        }
    const BarzelEvalResult* getVar( uint32_t varId ) const 
    {
        for( BarzelEvalProcFrameStack::const_reverse_iterator i = d_procFrameStack.rbegin(); i!= d_procFrameStack.rend(); ++i ) {
            const BarzelEvalResult* ber = i->getVar(varId);
            if( ber ) 
                return ber;
        }
        return 0;
    }
	const BarzelEvalResult* getPositionalArg( size_t pos ) const
		{ return ( d_procFrameStack.size()==0 ? 0 : d_procFrameStack.back().getPositionalArg(pos) ); }

	bool hasError() { return err != EVALERR_OK; }
	const uint8_t* setErr_GROW() { return( err = EVALERR_GROW, (const uint8_t*)0); } 
	const BELTrie& getTrie() const { return d_trie; }

    Barz& getBarz() { return d_barz; }
    const Barz& getBarz() const { return d_barz; }

	BarzelEvalContext( BarzelMatchInfo& mi, const StoredUniverse& uni, const BELTrie& trie, Barz& barz ) : 
		matchInfo(mi), universe(uni) , d_trie(trie),err(EVALERR_OK), d_barz(barz),d_procFrameStack(1)
	{}
    BarzelEvalContext& pushBarzelError( const char* err ) ; 
    const BarzelEvalProcFrame* getTopProcFrame() const 
        { return ( d_procFrameStack.size()==0 ? 0: &(d_procFrameStack.back()) ); }

    // const GlobalPools&  gp() const { return universe.getGlobalPools(); }
    const StoredUniverse&  getUniverse() const { return universe; }
private:
    BarzelEvalContext( const BarzelEvalContext& x ) :   
	    matchInfo(x.matchInfo),
	    universe(x.universe),
	    d_trie(x.d_trie),
	    err( x.err ),
        d_barz( x.d_barz ),
        d_procFrameStack(1)
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
	bool eval_comma(BarzelEvalResult&, BarzelEvalContext& ctxt ) const;
public:
    // returns true if this node if this is a LET node - let node assigns a variable 
    const BTND_Rewrite_Control* getControl() const 
        { return boost::get<BTND_Rewrite_Control>(&d_btnd); }

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

    const BTND_Rewrite_Control* isComma( ) const 
    {
        const BTND_Rewrite_Control* ctrl = getControl();
        return( (ctrl && ctrl->isComma()) ? ctrl:0 ); 
    }
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
