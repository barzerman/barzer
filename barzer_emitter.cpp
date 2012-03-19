#include <barzer_emitter.h>
#include <barzer_universe.h>
#include <ay/ay_debug.h>
namespace barzer {
    
//namespace{

bool BELParseTreeNode_PatternEmitter::produceSequence()
{
    if( !patternTree ) 
        return false;
    bool ret = patternTree->step();
    if (ret) {
        curVec.clear();
        varVec.clear();
    }
    //if(ret)
        //patternTree->yield(curVec);
    //bool ret = patternTree->step();
    //return ret;
    return ret;
}

void BELParseTreeNode_PatternEmitter::makePatternTree()
{
    VarVec v;
    patternTree = PatternEmitterNode::make(tree, v);
}
BELParseTreeNode_PatternEmitter::~BELParseTreeNode_PatternEmitter()
{
    delete patternTree;
}

//}//anon namespace
struct Leaf: public PatternEmitterNode {
    Leaf(const BTND_PatternData& d, const VarVec &v): data(d), vars(v) {}
    
    bool step() { return false; }

    void yield(BTND_PatternDataVec& vec, BELVarInfo &varInfo) const
    {
        varInfo.push_back(vars);
        vec.push_back(data);
    }
    
    const BTND_PatternData& data;
    VarVec vars;
};

struct IntermediateNode : public PatternEmitterNode {
    IntermediateNode(const BELParseTreeNode::ChildrenVec& children, VarVec &vars)
    {
        //AYLOGDEBUG(children.size());
        for(BELParseTreeNode::ChildrenVec::const_iterator it=children.begin(); it != children.end(); ++it)
            childs.push_back(make(*it, vars));
        //AYLOG(DEBUG) << "done";
    }
    
    virtual ~IntermediateNode()
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it)
            delete *it;
    }
    
protected:
    typedef std::vector<PatternEmitterNode*> ChildVec;
    ChildVec childs;
};

struct List: public IntermediateNode {
    List(const BELParseTreeNode::ChildrenVec& children, VarVec &v)
        : IntermediateNode(children, v) {}
    
    bool step()
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {

            if((*it) && (*it)->step()) {
                return true;
            }
        }
        return false;
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
            if( *it ) 
                (*it)->yield(vec, vinfo);
        }
    }
private:
};

struct Any: public IntermediateNode {
    Any(const BELParseTreeNode::ChildrenVec& children, VarVec &v)
        : IntermediateNode(children, v)
    {
        //AYLOGDEBUG(childs.size());
        position = childs.begin();
    }
    
    bool step()
    {
        if (position == childs.end()) return false;
        if((*position)->step())
            return true;
        
        ++position;
        
        if(position==childs.end()) {
            position = childs.begin();
            return false;
        }
        
        return true;
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        if (position != childs.end()) (*position)->yield(vec, vinfo);
    }
    
private:

    ChildVec::const_iterator position;
};

struct Opt: public IntermediateNode {
    Opt(const BELParseTreeNode::ChildrenVec& children, VarVec &v)
        : IntermediateNode(children, v)
    {
        selected = true;
    }
    
    bool step()
    {
        if (selected) {
            for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
                if((*it)->step()) {
                    return true;
                }
            }
            selected = false;
            return true;
        } else {
            selected = true;
            return false;
        }
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        if(selected) {
            for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it)
                (*it)->yield(vec, vinfo);
        }
    }
    
private:
    bool selected;
};

struct Perm: public IntermediateNode {
    Perm(const BELParseTreeNode::ChildrenVec& children, VarVec &v)
        : IntermediateNode(children, v)
    {
        for(unsigned i=0; i < childs.size(); ++i)
            currentPermutation.push_back(i);
    }
    
    bool step()
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
            if((*it)->step())
                return true;
        }
        return std::next_permutation(currentPermutation.begin(), currentPermutation.end());
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        for(PermVec::const_iterator it=currentPermutation.begin(); it != currentPermutation.end(); ++it)
            childs[*it]->yield(vec, vinfo);
    }
    
private:
    typedef std::vector<unsigned> PermVec;
    PermVec currentPermutation;
};

struct Tail: public IntermediateNode {
    Tail(const BELParseTreeNode::ChildrenVec& children, VarVec &v)
        : IntermediateNode(children, v)
    {
        endPosition = childs.begin() + 1;
    }
    
    bool step()
    {
        for(ChildVec::const_iterator it=childs.begin(); it != endPosition; ++it) {
            if((*it)->step())
                return true;
        }
        
        if(endPosition == childs.end()) {
            endPosition = childs.begin() + 1;
            return false;
        }
        
        ++endPosition;
        return true;
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        for(ChildVec::const_iterator it=childs.begin(); it != endPosition; ++it)
            (*it)->yield(vec, vinfo);
    }
    
private:
    ChildVec::const_iterator endPosition;
};
struct Subset: public IntermediateNode {

    Subset(const BELParseTreeNode::ChildrenVec& children, VarVec &v)
        : IntermediateNode(children, v),
        d_subsMax( (1<< children.size()) - 1 ),
        d_subsCurrent(1)
    {}
    
    bool isInCurrentSubset( uint32_t i ) const { return ( (1<< i) & d_subsCurrent ); }

    bool step()
    {
        for( uint32_t i = 0; i< childs.size(); ++i ) {
            if(isInCurrentSubset(i) && childs[i]->step())
                return true;
        }
        
        if(d_subsCurrent >= d_subsMax ) {
            d_subsCurrent = 1;
            return false;
        }

        ++d_subsCurrent;
        return true;
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        for(uint32_t i = 0; i< childs.size(); ++i ) {
            if( isInCurrentSubset(i) ) 
                childs[i]->yield(vec, vinfo);
        }
    }
private:
    uint32_t d_subsMax, d_subsCurrent;
};

//
    
PatternEmitterNode* PatternEmitterNode::make(const BELParseTreeNode& node, VarVec &vars)
{
    switch(node.btndVar.which()) {
        case BTND_StructData_TYPE: {
            const BTND_StructData &sdata = boost::get<BTND_StructData>(node.btndVar);
            //PatternEmitterNode* ret = 0;
            ay::vector_raii_p<VarVec> vp(vars);
             //vars.push_back(sdata.getVarId());
            if (sdata.hasVar()) vp.push(sdata.getVarId());
            switch(sdata.getType()) {
                case BTND_StructData::T_LIST:
                    return new List(node.child, vars);
                case BTND_StructData::T_ANY:
                    return new Any(node.child, vars);
                case BTND_StructData::T_OPT:
                    return new Opt(node.child, vars);
                case BTND_StructData::T_PERM:
                    return new Perm(node.child, vars);
                case BTND_StructData::T_TAIL:
                    return new Tail(node.child, vars);
                case BTND_StructData::T_SUBSET:
                    return new Subset(node.child, vars);
                default:
                    AYLOG(ERROR) << "Invalid BTND_StructData type: " << sdata.getType();
                    return new List(node.child, vars);
            }
            //if (sdata.hasVar()) vars.pop_back();
            //return ret;
        }
        case BTND_PatternData_TYPE:
            return new Leaf(boost::get<BTND_PatternData>(node.btndVar), vars);
    }
    
    // should never get here
    std::cerr << "something is definitely broken " << node.btndVar.which() << "\n";
    return NULL;
}
    
//// 
void BELReaderXMLEmit::addStatement(const barzer::BELStatementParsed& sp)
{
    BELParseTreeNode_PatternEmitter emitter( sp.pattern );
    int i =0;
    d_outStream << "<rule n=\"" <<  sp.getStmtNumber() <<"\">\n";
    do {
        const BTND_PatternDataVec& seq = emitter.getCurSequence();
        if( !seq.size() ) 
            continue;
        d_outStream << "    <pat n=\"" << i << "\">";
        for( BTND_PatternDataVec::const_iterator pi = seq.begin(); pi != seq.end();++pi ) 
            btnd_xml_print( d_outStream , getTrie(), *pi );
        d_outStream << "</pat>\n";
        i++;
        //AYLOG(DEBUG) << "path added";
    } while( emitter.produceSequence() );
    d_outStream << "</rule>" << std::endl;
}


BELReaderXMLEmit::BELReaderXMLEmit( BELTrie* t, std::ostream& os ) :
        BELReader( t, t->getGlobalPools(), &os ),
        d_outStream(os)
    {
    }


// BELReaderXMLEmitCounter implementation

BELReaderXMLEmitCounter::BELReaderXMLEmitCounter( BELTrie* t, std::ostream& os ) :
        BELReader( t, t->getGlobalPools(), &os ),
        d_outStream(os),
        d_maxConsidered(MAX_CONSIDERED),
        d_counter(0)
{
}

// split 
void BELReaderXMLEmitCounter::addStatement(const barzer::BELStatementParsed& sp)
{
    BELParseTreeNode node = sp.pattern;
    d_counter += power(node);
    // d_outStream <<"<rn>" << power(node) <<"</rn>" <<std::endl;
}


namespace {

template <typename T>
struct SumProductFunctor
{
    size_t type;
    size_t& x;
    const T& c;
    SumProductFunctor(const T& counter, size_t& xx, size_t node_t  ) :
        type(node_t),
        x(xx), 
        c(counter)  
    {
        x = (node_t == BTND_StructData::T_ANY || node_t == BTND_StructData::T_TAIL ? 0UL:1UL); 
    }
    size_t operator() (const barzer::BELParseTreeNode& node ) const
    {
        if (x > c.maxConsidered() ) return (x=BELReaderXMLEmitCounter::INF);
        size_t p = c.power(node);
        if (p > c.maxConsidered() ) return x=BELReaderXMLEmitCounter::INF;
        else 
        {   //can be optimised
            switch (type) {
                case BTND_StructData::T_ANY: return x = x + p;                      break;
                case BTND_StructData::T_TAIL: return x==0? x = p : x = p*(1+x);     break;
                case BTND_StructData::T_SUBSET: return x = x*(1 + p);               break;
                default: return p==0? x : x = x*p;
            }       
        }
    }
};


inline size_t factorial( size_t n )
{
  const size_t answer[] = {1,1,2,6,24,120,720,5040, 40320, 362880, 3628800, 39916800, 479001600};
  if (n > 12) return BELReaderXMLEmitCounter::INF;
  else return (answer[n] > BELReaderXMLEmitCounter::MAX_CONSIDERED) ? BELReaderXMLEmitCounter::INF : answer[n];
}

} // anon namespace 

size_t BELStatementParsed_EmitCounter::power(const barzer::BELParseTreeNode& node) const
{
    enum { MAX_EMIT_COUNTED = 1000000000 };
    switch (node.btndVar.which()) {
    case BTND_StructData_TYPE: {
        if (node.child.empty()) return 0;
        const BTND_StructData &sdata = boost::get<BTND_StructData>(node.btndVar);
        size_t p = 0;
        SumProductFunctor<BELStatementParsed_EmitCounter> aggr(*this, p, sdata.getType());
        if (BTND_StructData::T_TAIL == sdata.getType()) 
            for ( int i = node.child.size() - 1; i >= 0; i-- ) aggr(node.child[i]); //reverse order important!
        else   std::for_each( node.child.begin(), node.child.end(), aggr);
     
        if (p > MAX_EMIT_COUNTED) return BELReaderXMLEmitCounter::INF;
        switch (sdata.getType()) {
            case BTND_StructData::T_LIST:   /*nothing*/;                            break;
            case BTND_StructData::T_ANY:    /*nothing*/;                            break;
            case BTND_StructData::T_OPT:    p++;                                    break;  
            case BTND_StructData::T_PERM:   p *= factorial(node.child.size());      break;
            case BTND_StructData::T_TAIL:   /*nothing*/;                            break;
            case BTND_StructData::T_SUBSET: p--;                                    break;
            default:
                AYLOG(ERROR) << "Invalid BTND_StructData type: " << sdata.getType();
                return 0;
        }
            ////debug: std::cout << "node="<< sdata.getType() <<" p=" <<p << std::endl;
            return ((p > MAX_EMIT_COUNTED)? BELReaderXMLEmitCounter::INF : p );
        }
        case BTND_PatternData_TYPE:
            return 1;
    }
    AYLOG(ERROR) << "Smth definitly broken.Invalid BTND_TYPE type: " << node.btndVar.which();
    return 0;
}

/// return BELReaderXMLEmitCounter::INF if the result exceedes BELReaderXMLEmitCounter::MAX_CONSIDERED
size_t BELReaderXMLEmitCounter::power(const barzer::BELParseTreeNode& node) const
    { return BELStatementParsed_EmitCounter(getTriePtr(),d_maxConsidered).power( node ); }

} // barzer namespace 
