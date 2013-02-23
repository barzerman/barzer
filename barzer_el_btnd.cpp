/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_el_btnd.h>
#include <barzer_emitter.h>
#include <barzer_el_trie.h>

#include <algorithm>
#include <ay/ay_logger.h>
#include <ay/ay_util.h>
#include <barzer_universe.h>
#include <barzer_server_response.h>

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

std::ostream&  BTND_StructData::printXML( std::ostream& fp, const BELTrie& trie  ) const
{
	fp << getXMLTag();
	if( hasVar() ) {
		trie.printVariableName( fp << " v=\"", varId ) << "\"";
	}
    if( type == T_FLIP ) {
        fp << " type=\"flip\"";
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

		d_fp << '>';
		const char* txt = d_trie.getGlobalPools().decodeStringById_safe( d.stringId );
        if( txt ) {
            xmlEscape( txt, d_fp );
        }
		d_fp << "</t>";

		return tag;
	}
	const char* operator()( const BTND_Pattern_Punct& d ) 
	{
		const char* tag = "t";
		d_fp << "<t>" << ( isascii(d.theChar) ? char(d.theChar) : ' ' ) << "</t>";
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
		//d_fp << "<erc";
		d.printXML(d_fp,d_trie.getGlobalPools());
		//d_fp << TAG_CLOSE_STR;
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
namespace {

struct NameFromPatternGetter :  public boost::static_visitor<void> {
    mutable std::stringstream  d_str;
    const GlobalPools& d_gp;
    mutable bool  d_hasStuff;
    
    NameFromPatternGetter(const GlobalPools& g ) : 
        d_gp(g), d_hasStuff(false) {}
    
    void operator()( const BTND_Pattern_Token& p ) const {
        const char* str = d_gp.string_resolve( p.getStringId() );
        if( str ) {
            if( d_hasStuff )
                d_str << " " << str ;
            else  {
                d_str << str;
                d_hasStuff= true;
            }
        }
    }
    void operator()( const BTND_Pattern_Punct& p ) const {
        char c = static_cast<char>(p.getChar());
        if( ispunct(c) ) 
            d_str << c;
    }
    void operator()( const BTND_PatternData& p ) const {
       boost::apply_visitor( *this, p );
    } 

    template <typename T>
    void operator()( const T& ) const { }  // we do nothing for non pattern types
    void addToName(  const BELParseTreeNode& tn ) const {
        const BTND_StructData* sd = tn.getStructData();
        bool isAny ( sd ? sd->isANY() : false );
        
        for( BELParseTreeNode::ChildrenVec::const_iterator ci = tn.child.begin(); ci!= tn.child.end(); ++ci ) {
            if( isAny && (ci-tn.child.begin()) ) 
                break;
            const BTND_StructData* tmpSd = ci->getStructData(); 
            if( tmpSd ) 
                addToName( *ci );
            else {
                boost::apply_visitor( *this, ci->getVar() );
            }
        }
    }
    
    void computeName( std::string& str, const BELParseTreeNode& tn  ) const
    {
        addToName(tn);
        str = d_str.str();
    }
}; 

}

void BELParseTreeNode::getDescriptiveNameFromPattern_simple( std::string& outStr, const GlobalPools& gp )
{ NameFromPatternGetter(gp).computeName( outStr, *this ); }

} // namespace barzer
