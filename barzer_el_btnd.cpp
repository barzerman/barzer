#include <barzer_el_btnd.h>
#include <barzer_el_trie.h>

#include <algorithm>
#include <ay/ay_logger.h>
#include <ay/ay_util.h>
#include <barzer_universe.h>

namespace barzer {

#define TAG_CLOSE_STR "/>" 

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


std::ostream&  BTND_Rewrite_Select::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
    return ( fp << "select." << ctxt.printableString( varId ) );
}

std::ostream&  BTND_Rewrite_Case::print( std::ostream& fp , const BELPrintContext& ctxt) const
{
    return ( fp << "case." << ctxt.printableString( ltrlId ) );
}


std::ostream&  BTND_Rewrite_None::print( std::ostream& fp , const BELPrintContext& ) const
{
	return (fp << "RwrNone" );
}

std::ostream&  BTND_StructData::printXML( std::ostream& fp, const BELTrie& trie  ) const
{
	fp << getXMLTag();
	if( hasVar() ) {
		trie.printVariableName( fp << " v=\"", varId ) << "\"";
	}
	return fp;
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

std::ostream& BTND_Pattern_DateTime::printXML( std::ostream& fp, const GlobalPools& gp ) const 
{
	fp << "<dtim";
	switch(type) {
	case T_ANY_DATETIME:
		break;
	case T_ANY_FUTURE_DATETIME: 
		fp << " f=\"y\"";
		break;
	case T_ANY_PAST_DATETIME: 
		fp << " p=\"y\"";
		break;
	case T_DATETIME_RANGE: 
		if( lo.isValid() ) {
			if( lo.hasDate() ) {
				fp << " dl=\"" << lo.date.getLong() << "\"";
			}
			if( lo.hasTime() ) {
				fp << " tl=\"" << lo.timeOfDay.getLong() << "\"";
			}
		}
		if( hi.isValid() ) {
			if( hi.hasDate() ) {
				fp << " dh=\"" << hi.date.getLong() << "\"";
			}
			if( hi.hasTime() ) {
				fp << " th=\"" << hi.timeOfDay.getLong() << "\"";
			}
		}
		break;
	}
	fp << TAG_CLOSE_STR;
	return fp;
}
std::ostream& BTND_Pattern_Time::printXML( std::ostream& fp, const GlobalPools& gp ) const 
{
	fp << "<time";
	switch(type) {
	case T_ANY_TIME:
		break;
	case T_ANY_FUTURE_TIME:
		fp << " f=\"y\"";
		break;
	case T_ANY_PAST_TIME: 
		fp << " p=\"y\"";
		break;
	case T_TIMERANGE: 
		fp << " l=\"" << getLoLong() << "\" h=\"" << getHiLong() << "\"";
		break;
	}
	fp << TAG_CLOSE_STR;
	return fp;
}
std::ostream& BTND_Pattern_Date::printXML( std::ostream& fp, const GlobalPools& gp ) const 
{
	fp << "<date";
	switch( type ) {
	case T_ANY_DATE:
		break;
	case T_ANY_FUTUREDATE:
		fp << " f=\"y\"";
		break;
	case T_ANY_PASTDATE:
		fp << " p=\"y\"";
		break;
	case T_TODAY: // this seems to be unused
		break;

	case T_DATERANGE:
		if( isLoSet() ) {
			fp << " l=\"" << lo << "\"";
		}
		if( isHiSet() ) {
			fp << " h=\"" << hi << "\"";
		}
		break;
	}
	fp << TAG_CLOSE_STR;

	return fp;
}
std::ostream& BTND_Pattern_ERC::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<erc";
	if( isRangeValid() ) {
		fp << " r=\"" << d_erc.getRange().geXMLtAttrValueChar() << "\"";
	} else if(isMatchBlankRange()) {
		fp << " br=\"y\"";
	}
	if( isEntityValid() ) {
		fp << " c=\"" << d_erc.getEntity().eclass.ec << "\"";
		if( d_erc.getEntity().eclass.subclass ) {
			fp << " s=\"" << d_erc.getEntity().eclass.subclass << "\"";
		}
		if( d_erc.getEntity().tokId != 0xffffffff ) {
			fp << " t=\"" << gp.decodeStringById_safe( d_erc.getEntity().tokId ) << "\"";
		}
	} else if(isMatchBlankEntity()){
		fp << " be=\"y\"";
	}
	if( isUnitEntityValid() ) {
		fp << " uc=\"" << d_erc.getUnitEntity().eclass.ec << "\"";
		if( d_erc.getEntity().eclass.subclass ) {
			fp << " us=\"" << d_erc.getUnitEntity().eclass.subclass << "\"";
		}
		if( d_erc.getEntity().tokId != 0xffffffff ) {
			fp << " ut=\"" << gp.decodeStringById_safe( d_erc.getUnitEntity().tokId ) << "\"";
		}
	}

	return fp << TAG_CLOSE_STR;
}
std::ostream& BTND_Pattern_Entity::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<ent";
	if( d_ent.eclass.ec ) {
		fp << " c=\"" << d_ent.eclass.ec << "\"";
	}
	if( d_ent.eclass.subclass ) {
		fp << " s=\"" << d_ent.eclass.subclass << "\"";
	}
	if( d_ent.eclass.subclass ) {
		fp << " s=\"" << d_ent.eclass.subclass << "\"";
	}
	if( d_ent.tokId != 0xffffffff ) {
		fp << " t=\"" << gp.decodeStringById_safe( d_ent.tokId )   << "\"";
	}
	fp << TAG_CLOSE_STR;
	return fp;
}
std::ostream& BTND_Pattern_Range::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<range";
	if( range().isValid() ) 
		fp << " t=\"" << range().geXMLtAttrValueChar()  << "\"";
	if( isModeVal() ) {
		fp << " m=\"v\"";
	}
	const BarzerRange::Entity* re = d_range.getEntity();
	if( re ) {
		fp << " ec=\"" << re->first.eclass.ec << "\" es=\"" << re->first.eclass.subclass << "\"";
		if( re->first.tokId != 0xffffffff ) {
			fp << " ei1=\"" << gp.decodeStringById_safe(re->first.tokId ) << "\"";
		}
		if( re->second.tokId != 0xffffffff ) {
			fp << " ei1=\"" << gp.decodeStringById_safe(re->second.tokId ) << "\"";
		}
	}
	return fp << TAG_CLOSE_STR;
}
std::ostream& BTND_Pattern_ERCExpr::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<ercexpr";
	return fp << TAG_CLOSE_STR;
}
std::ostream& BTND_Pattern_Number::printXML( std::ostream& fp, const GlobalPools& gp ) const
{
	fp << "<n";
	if( d_asciiLen ) {
		fp << " w=\"" <<  d_asciiLen << '"';
	}
	switch( type ) {
	case T_ANY_INT: break;
	case T_ANY_REAL:
		fp << " r =\"y\"";
		break;
	case T_RANGE_INT: 
		fp << " l=\"" << range.integer.lo << "\" h=\"" << range.integer.hi << "\""; break;
	case T_RANGE_REAL: 
		fp << "l=\"" << range.real.lo << "\" h=\"" << range.real.hi << "\""; break;
	case T_ANY_NUMBER: break;
	}
	return fp << TAG_CLOSE_STR;
}
bool BTND_Pattern_Number::operator() ( const BarzerNumber& num ) const
{
	uint8_t len = getAsciiLen();
	if(len && len!= num.getAsciiLen()) 
		return false;
		
	switch( type ) {
	case T_ANY_NUMBER: return true;
	case T_ANY_INT: return num.isInt();
	case T_ANY_REAL: return true;

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

/// print methods 
namespace {

struct BTND_Base_print_XML : public boost::static_visitor<const char*> {
	bool d_closeTag;  // when tag has no children nor cdata 
	std::ostream& d_fp;
	const BELTrie& d_trie;

	void beginTag( ) { d_fp << '<'; } 
	void endTag( bool hasText = false ) { d_fp << ( (hasText || d_closeTag) ? TAG_CLOSE_STR : ">" ); }
	void printClosingTag( const char* tag) { d_fp << "</" << tag << '>' ; }

	BTND_Base_print_XML( std::ostream& fp, bool closeTag, const BELTrie& trie ) :
		d_closeTag(closeTag), d_fp(fp), d_trie(trie) 
	{}
};

struct BTND_PatternData_print_XML : public BTND_Base_print_XML {
	BTND_PatternData_print_XML( std::ostream& fp, bool closeTag, const BELTrie& trie ) :
		BTND_Base_print_XML(fp,closeTag, trie ) {}
	template <typename T>
	const char* operator() ( const T& d ) 
	{ 
		const char* tag = "undefined";

		return( d_fp << tag, tag );
	}
	
	const char* operator()( const BTND_Pattern_None& d ) 
	{
		return "pat_none";
	}
	const char* operator()( const BTND_Pattern_Token& d ) 
	{
		const char* tag = "t";
		d_fp <<"<t";
		if( !d.doStem ) {
			d_fp << " s=\"n\"";
		}
		d_fp << '>' << d_trie.getGlobalPools().decodeStringById_safe( d.stringId ) << "</t>";

		return tag;
	}
	const char* operator()( const BTND_Pattern_Punct& d ) 
	{
		const char* tag = "t";
		d_fp << "<t>" << ( isascii(d.theChar) ? d.theChar : ' ' ) << "</t>";
		return tag;
	}
	const char* operator()( const BTND_Pattern_Number& d ) 
	{
		const char* tag = "n";
		d.printXML(d_fp,d_trie.getGlobalPools());
		return tag;
	}
	const char* operator()( const BTND_Pattern_Date& d ) 
	{
		const char* tag = "date";
		d.printXML( d_fp,d_trie.getGlobalPools() );
		return tag;
	}
	const char* operator()( const BTND_Pattern_Time& d ) 
	{
		const char* tag = "time";
		d.printXML( d_fp,d_trie.getGlobalPools() );
		return tag;
	}
	const char* operator()( const BTND_Pattern_DateTime& d ) 
	{
		const char* tag = "dtim";
		d.printXML( d_fp,d_trie.getGlobalPools() );
		return tag;
	}
	const char* operator()( const BTND_Pattern_StopToken& d ) 
	{
		const char* tag = "t";
		d_fp << "<t t=\"f\">" << d_trie.getGlobalPools().decodeStringById_safe( d.stringId ) << "</t>";
		return tag;
	}
	const char* operator()( const BTND_Pattern_Entity& d ) 
	{
		const char* tag = "ent";
		d.printXML( d_fp,d_trie.getGlobalPools() );
		return tag;
	}
	const char* operator()( const BTND_Pattern_ERCExpr& d ) 
	{
		const char* tag = "ercexpr";
		d.printXML( d_fp,d_trie.getGlobalPools() );
		return tag;
	}
	const char* operator()( const BTND_Pattern_ERC& d ) 
	{
		const char* tag = "erc";
		d_fp << "<erc";
		d.printXML(d_fp,d_trie.getGlobalPools());
		d_fp << TAG_CLOSE_STR;
		return tag;
	}
	const char* operator()( const BTND_Pattern_Range& d ) 
	{
		const char* tag = "range";
		d.printXML( d_fp,d_trie.getGlobalPools() );
		return tag;
	}
};

struct BTNDVariant_print_XML : public BTND_Base_print_XML {
	BTNDVariant_print_XML( std::ostream& fp, bool closeTag, const BELTrie& trie ) : 
		BTND_Base_print_XML(fp,closeTag, trie ) {}
	
	const char* operator()( const BTND_None& ) { 
		return ""; 
	}

	const char* operator()( const BTND_PatternData& d ) { 
		BTND_PatternData_print_XML vis(d_fp, d_closeTag, d_trie );
		return boost::apply_visitor( vis, d );
	}
	const char* operator()( const BTND_StructData& d ) { 
		const char* tag = d.getXMLTag();
		beginTag();
		d.printXML( d_fp, d_trie );
		endTag();
		return tag;
	}
	template <typename T> 
	const char* operator()( const T& ) { 
		const char* tag = "unimplemented";
		d_fp << "<" << tag << "/>";
		return tag;
	}
};

} // anonymous namespace ends 

std::ostream& btnd_xml_print( std::ostream& fp, const BELTrie& trie, const BTND_PatternData& d )
{
	BTND_PatternData_print_XML vis(fp, true, trie );
	boost::apply_visitor( vis, d );
	return fp;
}

std::ostream&  BELParseTreeNode::printBarzelXML( std::ostream& fp, const BELTrie& trie ) const
{
	BTNDVariant_print_XML vis(fp, (!child.size()), trie );
	const char* tag = boost::apply_visitor( vis, btndVar );
	if( child.size() ) {
		for( ChildrenVec::const_iterator i = child.begin(); i!= child.end(); ++i ) {
			i->printBarzelXML(fp,trie);
		}
		vis.printClosingTag( tag );
	}

	return fp;
}

} // namespace barzer
