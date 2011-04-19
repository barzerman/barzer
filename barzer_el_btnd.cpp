#include <barzer_el_btnd.h>
#include <barzer_el_trie.h>

#include <algorithm>
#include <ay/ay_logger.h>

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
	ENUMCASE(StopToken)
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



namespace {
    
struct Leaf: public PatternEmitterNode {
    Leaf(const BTND_PatternData& d): data(d) {}
    
    bool step() { return false; }
    //bool step() { return true; }
    void yield(BTND_PatternDataVec& vec) const { vec.push_back(data); }
    
    const BTND_PatternData& data;
};

struct IntermediateNode : public PatternEmitterNode {
    IntermediateNode(const BELParseTreeNode::ChildrenVec& children)
    {
    	//AYLOGDEBUG(children.size());
        for(BELParseTreeNode::ChildrenVec::const_iterator it=children.begin(); it != children.end(); ++it)
            childs.push_back(make(*it));
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
    List(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children) {}
    
    bool step()
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
        	if((*it)->step()) {
                return true;
        	}
        }
        return false;
    }
    
    void yield(BTND_PatternDataVec& vec) const
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
            (*it)->yield(vec);
        }
    }
private:
};

struct Any: public IntermediateNode {
    Any(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children)
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
    
    void yield(BTND_PatternDataVec& vec) const
    {
    	if (position != childs.end()) (*position)->yield(vec);
    }
    
private:

    ChildVec::const_iterator position;
};

struct Opt: public IntermediateNode {
    Opt(const BELParseTreeNode::ChildrenVec& children): IntermediateNode(children)
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
    
    void yield(BTND_PatternDataVec& vec) const
    {
        if(selected) {
            for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it)
                (*it)->yield(vec);
        }
    }
    
private:
    bool selected;
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
    if (ret) curVec.clear();
    //if(ret)
        //patternTree->yield(curVec);
    //bool ret = patternTree->step();
    //return ret;
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

/// print methods 

std::ostream&  BTND_Pattern_Number::print( std::ostream& fp, const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_INT:
		fp << "AnyInt";
		break;
	case T_ANY_REAL:
		fp << "AnyReal";
		break;
	case T_RANGE_INT:
		fp << "RangeInt[" << range.integer.lo << "," << range.integer.hi << "]";
		break;
	case T_RANGE_REAL:
		fp << "RangeReal[" << range.real.lo << "," << range.real.hi << "]";
		break;
	default:
		return ( fp << "NumUnknown" );	
	}
	return fp; 
}

std::ostream&  BTND_Pattern_Punct::print( std::ostream& fp, const BELPrintContext&  ) const
{
	return ( fp << "Punct (" << (char)theChar << ")" );
}
std::ostream&  BTND_Pattern_Wildcard::print( std::ostream& fp , const BELPrintContext& ) const
{
	return ( fp << "Wildcard(" << minTerms << "," << maxTerms << ")");
}

std::ostream&  BTND_Pattern_Date::print( std::ostream& fp , const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_DATE:
		return (fp << "AnyDate");
	case T_ANY_FUTUREDATE:
		return (fp << "AnyFutureDate");
	case T_ANY_PASTDATE:
		return (fp << "AnyPastDate");

	case T_DATERANGE:
		return ( fp << "Date[" << lo << "," << hi << "]" );
	}
	return (fp << "UnknownDate" );
}
std::ostream&  BTND_Pattern_Time::print( std::ostream& fp , const BELPrintContext& ) const
{

	switch( type ) {
	case T_ANY_TIME: return( fp << "AnyTime" );
	case T_ANY_FUTURE_TIME: return( fp << "AnyFutureTime" );
	case T_ANY_PAST_TIME:return( fp << "AnyPastTime" );
	case T_TIMERANGE:return( fp << "Time["  << lo << "," << hi << "]");
	}
	return ( fp << "UnknownTime" );
}
std::ostream&  BTND_Pattern_DateTime::print( std::ostream& fp , const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_DATETIME: return (fp << "AnyDateTime" );
	case T_ANY_FUTURE_DATETIME: return (fp << "FutureDateTime" );
	case T_ANY_PAST_DATETIME: return (fp << "PastDateTime" );
	case T_DATETIME_RANGE: return (fp << "DateTime[" << dlo << "-" << tlo << "," << dhi << "-" << thi << "]");
	}
	return (fp << "DateTimeUnknown");
}
std::ostream&  BTND_Pattern_CompoundedWord::print( std::ostream& fp , const BELPrintContext& ) const
{
	return ( fp << "compword(" << std::hex << compWordId << ")");
}
std::ostream&  BTND_Pattern_Token::print( std::ostream& fp , const BELPrintContext& ctxt ) const
{
	return ( fp << "token[" << ctxt.printableString( stringId ) << "]");
}
std::ostream&  BTND_Pattern_None::print( std::ostream& fp , const BELPrintContext& ) const
{
	return (fp << "<PatternNone>");
}

std::ostream&  BTND_Rewrite_Variable::print( std::ostream& fp , const BELPrintContext& ctxt ) const
{
	if( byName ) {
		return ( fp << "$" << ctxt.printableString( varId ) ) ;
	} else {
		return ( fp << "$" << varId );
	}
}
std::ostream&  BTND_Rewrite_Function::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
	return ( fp << "func." << ctxt.printableString( nameId ) );
}
std::ostream&  BTND_Rewrite_None::print( std::ostream& fp , const BELPrintContext& ) const
{
	return (fp << "RwrNone" );
}

std::ostream&  BTND_StructData::print( std::ostream& fp , const BELPrintContext& ) const
{
	const char* str = "Unknown";
	switch( type ) {
	case T_LIST: str = "List" ; break;
	case T_ANY: str = "Any" ; break;
	case T_OPT: str = "Opt" ; break;
	case T_PERM: str = "Perm" ; break;
	case T_TAIL: str = "Tail" ; break;
	default: str = "Unknown"; break;
	}
	return ( fp << "Struct." << str );
}
std::ostream&  BTND_None::print( std::ostream& fp , const BELPrintContext& ) const
{
	return ( fp << "None" ); 
}

} // namespace barzer
