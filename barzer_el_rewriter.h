
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <stack>
//#include <barzer_emitter.h>
#include <barzer_el_parser.h>
#include <barzer_el_chain.h>
#include <barzer_el_eval.h>
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
struct RequestEnvironment;

/// unique at least per BarzelTrie (most likely globally unique)
/// the pool stores binary buffers with barzel rewrite instructions
/// these buffers are used to build BarzelRewriteTree objects on the fly 
class BarzelRewriterPool {
private:
	// BufAndSizeVec encVec;
    std::vector< BarzelEvalNode > rawNodesVec;
    
    
	uint32_t poolNewRawNode( const BarzelEvalNode& evn );
public:
	static int  encodeParseTreeNode( BarzelEvalNode& evNode, const BELParseTreeNode& ptn );

	void clear();
	~BarzelRewriterPool();
	BarzelRewriterPool( size_t reserveSz ) 
		{ rawNodesVec.reserve( reserveSz ) ; }


	enum {
		ERR_OK,
		ERR_ENCODING_FAILED, // input tree is invalid
		ERR_ENCODING_INCONSISTENT // some internal inconsistency encountered
	};
	// returns one of the ERR_XXX enums
	int produceTranslation( BarzelTranslation& , const BELParseTreeNode& ptn );

    const BarzelEvalNode* getRawNode( const BarzelTranslation& trans ) const;
    BarzelEvalNode* getRawNode( const BarzelTranslation& trans );
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
    int d_confidenceBoost;
    
public:
	BarzelEvalResult() : 
        d_val(1), 
        d_unmatchable(0),
        d_confidenceBoost(0)
    {}

    void boostConfidence( int x = 1 ) { d_confidenceBoost+=x; }
    int  getConfidenceBoost() const { return d_confidenceBoost; }
    void  setConfidenceBoost( int x ) { d_confidenceBoost=x; }

    int  getUnmatchability() const { return d_unmatchable; }
    
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
	template <typename T> void setBeadData( const T& t ) { 
        if( d_val.size() > 1 ) 
            d_val.resize(1);
        d_val[0] = t; 
    }

	template <typename T> void pushBeadData( const T& t ) { 
        d_val.push_back( BarzelBeadData( BarzelBeadAtomic()) );
        boost::get<BarzelBeadAtomic>(d_val.back()).setData(t);
    }
	template <typename T> void pushOrSetBeadData( size_t j, const T& t ) { 
        if( !j ) 
            d_val[0] = BarzelBeadAtomic(t);
        else 
            pushBeadData(t);
    }
    
    std::ostream& print( std::ostream& fp ) const
        { return ( fp << getBeadData() ); }
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

// typedef boost::unordered_map< uint32_t, BarzelEvalResult > VariableEvalMap;
typedef std::map< uint32_t, BarzelEvalResult > VariableEvalMap;

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
    
    void bindValue( uint32_t varId, const BarzelEvalResult& ber )
    {
        auto i = d_varMap.find(varId);
        if( i == d_varMap.end() ) {
            d_varMap.insert( VariableEvalMap::value_type(varId, ber) );
        } else
            i->second=ber;
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

class BarzelEvalContext {
public:
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
                  context.d_procFrameStack.pop_back();
        }
	};
    // let var 
    void bindVar( uint32_t varId, const BarzelEvalResult& val ) 
        { 
            if( !d_procFrameStack.empty() ) {
                d_procFrameStack.back().bindValue( varId, val );
            }
        }
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

    const RequestEnvironment* getRequestEnvironment() ;
    const BarzerDateTime* getNowPtr() const;
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
    void  pushError( const char* error, const char* e2=0 );
    const char* resolveStringInternal( uint32_t i ) const;
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

} // barzer namespace 
