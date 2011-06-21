#include <barzer_el_btnd.h>
#include <barzer_el_trie.h>

#include <algorithm>
#include <ay/ay_logger.h>
#include <ay/ay_util.h>

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
	case BTND_Rewrite_MkEnt_TYPE: return "MkEnt";
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
        	if((*it)->step()) {
                return true;
        	}
        }
        return false;
    }
    
    void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const
    {
        for(ChildVec::const_iterator it=childs.begin(); it != childs.end(); ++it) {
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
		d_subsMax( (1<< children.size()) ),
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

} // namespace {
    
PatternEmitterNode* PatternEmitterNode::make(const BELParseTreeNode& node, VarVec &vars)
{
    switch(node.btndVar.which()) {
        case BTND_StructData_TYPE: {
        	const BTND_StructData &sdata = boost::get<BTND_StructData>(node.btndVar);
        	//PatternEmitterNode* ret = 0;
        	ay::vector_raii_p<VarVec> vp(vars);
        	 //vars.push_back(sdata.getVarId());
        	if (sdata.hasVar())	vp.push(sdata.getVarId());
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
            }
            //if (sdata.hasVar()) vars.pop_back();
            //return ret;
        }
        case BTND_PatternData_TYPE:
            return new Leaf(boost::get<BTND_PatternData>(node.btndVar), vars);
    }
    
    // should never get here
    AYTRACE("something is definitely broken");
    return NULL;
}

bool BELParseTreeNode_PatternEmitter::produceSequence()
{
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

/// print methods 

std::ostream&  BTND_Pattern_Number::print( std::ostream& fp, const BELPrintContext& ) const
{
	switch( type ) {
	case T_ANY_NUMBER:
		fp << "AnyNum";
		break;
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
	case T_DATETIME_RANGE: return (fp << "DateTime[" << lo << "-" << hi << "]");
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
	switch( idMode ) {
	case MODE_WC_NUMBER: // wildcard number
		return( fp << "$" << varId  );
	case MODE_VARNAME:   // variable name 
		return( fp << "$" << ctxt.printVariableName( fp, varId ) );
	case MODE_PATEL_NUMBER: // pattern element number
		return( fp << "p$" << varId );
	}
	return ( fp << "bad$" << varId);
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
	case T_SUBSET: str = "Subset" ; break;
	default: str = "Unknown"; break;
	}
	return ( fp << "Struct." << str );
}
std::ostream&  BTND_None::print( std::ostream& fp , const BELPrintContext& ) const
{
	return ( fp << "None" ); 
}

bool BTND_Pattern_Number::operator() ( const BarzerNumber& num ) const
{
	switch( type ) {
	case T_ANY_NUMBER:
		return true;
	case T_ANY_INT:
		return num.isInt();
	case T_ANY_REAL:
		return true;

	case T_RANGE_INT:
		if( num.isInt() ) {
			int n = num.getInt_unsafe();
			return( n <= range.integer.hi && n>=range.integer.lo );
		} else 
			return false;
	case T_RANGE_REAL:
		if( num.isReal() ) {
			double n = num.getReal_unsafe();
			return( n <= range.integer.hi && n>=range.integer.lo );
		} else if( num.isInt() ) {
			double n = num.getInt_unsafe();
			return( n <= range.integer.hi && n>=range.integer.lo );
		} else
			return false;
		break;
	default: 
		return false;
	}
	return false;
}

} // namespace barzer
