#include <barzer_el_btnd.h>

#include <algorithm>

namespace barzer {

/// name decodings for BTND types per class


const char* BTNDDecode::typeName_Pattern ( int x ) 
{
#define ENUMCASE(x) case BTND_Pattern_##x##_TYPE: return #x;
	switch(x) {
	ENUMCASE(None)
	ENUMCASE(Token)
	ENUMCASE(Punct)
	ENUMCASE(CompoundedWord)
	ENUMCASE(Number)
	ENUMCASE(Wildcard)
	ENUMCASE(Date)
	ENUMCASE(Time)
	ENUMCASE(DateTime)
	default: 
		return "PatternUNDEF";
	}
#undef ENUMCASE
}

const char* BTNDDecode::typeName_Rewrite ( int x ) 
{
	switch(  x ) {
	case BTND_Rewrite_None_TYPE: return "RewriteNone";
	case BTND_Rewrite_Literal_TYPE: return "Literal";
	case BTND_Rewrite_Number_TYPE: return "Number";
	case BTND_Rewrite_Variable_TYPE: return "Variable";
	case BTND_Rewrite_Function_TYPE: return "Function";
	default: return "RewriteUnknown";
	}
}

const char* BTNDDecode::typeName_Struct ( int ) 
{
	return "BarzelStruct";
}

struct PatternEmitterNode {
    virtual bool step() = 0;
    virtual void yield(BTND_PatternDataVec& vec) const = 0;
    virtual ~PatternEmitterNode() {}
    
    static PatternEmitterNode* make(const BELParseTreeNode& node);
};

namespace {
    
struct Leaf: public PatternEmitterNode {
    Leaf(const BTND_PatternData& d): data(d) {}
    
    bool step() { return false; }
    void yield(BTND_PatternDataVec& vec) const { vec.push_back(data); }
    
    const BTND_PatternData& data;
};

struct IntermediateNode : public PatternEmitterNode {
    IntermediateNode(const BELParseTreeNode::ChildrenVec& children)
    {
        for(BELParseTreeNode::ChildrenVec::const_iterator it=children.begin(); it != children.end(); ++it)
            childs.push_back(make(*it));
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
    List(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children) {}
    
    bool step()
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
            if((*it)->step())
                return true;
        }
        return false;
    }
    
    void yield(BTND_PatternDataVec& vec) const
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it)
            (*it)->yield(vec);
    }
};

struct Any: public IntermediateNode {
    Any(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children)
    {
        position = childs.begin();
    }
    
    bool step()
    {
        if((*position)->step())
            return true;
        
        ++position;
        
        if(position==childs.end()) {
            position = childs.begin();
            return false;
        }
        
        return true;
    }
    
    void yield(BTND_PatternDataVec& vec) const
    {
        (*position)->yield(vec);
    }
    
private:
    ChildVec::const_iterator position;
};

struct Opt: public IntermediateNode {
    Opt(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children)
    {
        selected = true;
        position = childs.begin();
    }
    
    bool step()
    {
        if(selected) {
            if((*position)->step())
                return true;
            
            ++position;
            
            if(position==childs.end())
                selected = false;
        
            return true;
        }
        else {
            position = childs.begin();
            selected = true;
            return false;
        }
    }
    
    void yield(BTND_PatternDataVec& vec) const
    {
        if(selected)
            for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it)
                (*it)->yield(vec);
    }
    
private:
    bool selected;
    ChildVec::const_iterator position;
};

struct Perm: public IntermediateNode {
    Perm(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children)
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
    
    void yield(BTND_PatternDataVec& vec) const
    {
        for(PermVec::const_iterator it=currentPermutation.begin(); it != currentPermutation.end(); ++it)
            childs[*it]->yield(vec);
    }
    
private:
    typedef std::vector<unsigned> PermVec;
    PermVec currentPermutation;
};

struct Tail: public IntermediateNode {
    Tail(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children)
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
    
    void yield(BTND_PatternDataVec& vec) const
    {
        for(ChildVec::const_iterator it=childs.begin(); it != endPosition; ++it)
            (*it)->yield(vec);
    }
    
private:
    ChildVec::const_iterator endPosition;
};

} // namespace {
    
PatternEmitterNode* PatternEmitterNode::make(const BELParseTreeNode& node)
{
    switch(node.btndVar.which()) {
        case BTND_StructData_TYPE:
            switch(boost::get<BTND_StructData>(node.btndVar).type) {
                case BTND_StructData::T_LIST:
                    return new List(node.child);
                case BTND_StructData::T_ANY:
                    return new Any(node.child);
                case BTND_StructData::T_OPT:
                    return new Opt(node.child);
                case BTND_StructData::T_PERM:
                    return new Perm(node.child);
                case BTND_StructData::T_TAIL:
                    return new Tail(node.child);
            }
            break;
        case BTND_PatternData_TYPE:
            return new Leaf(boost::get<BTND_PatternData>(node.btndVar));
    }
    
    // should never get here
    AYTRACE("something is definitely broken");
    return NULL;
}

bool BELParseTreeNode_PatternEmitter::produceSequence()
{
    bool ret = patternTree->step();
    
    curVec.clear();
    if(ret)
        patternTree->yield(curVec);
    
    return ret;
}

void BELParseTreeNode_PatternEmitter::makePatternTree()
{
    patternTree = PatternEmitterNode::make(tree);
}
BELParseTreeNode_PatternEmitter::~BELParseTreeNode_PatternEmitter()
{
    delete patternTree;
}


} // namespace barzer
