#include <barzer_el_btnd.h>

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


// don't pay too much attention to it, it's just a testground
void BELParseTreeNode_PatternEmitter::pushAll( const BELParseTreeNode& t )
{

	t.print( std::cerr, 1 );
	const BELParseTreeNode::ChildrenVec& cv = t.child;

	switch( t.getNodeData().which() ) {
	case BTND_None_TYPE:
		AYDEBUG("unknown node type omg wtf");
		break;
	case BTND_StructData_TYPE:
		AYDEBUG("struct");
		AYDEBUG(t.getStructData()->type);
		AYDEBUG(cv.size());
		for( BELParseTreeNode::ChildrenVec::const_iterator ci = cv.begin(); ci
				!= cv.end(); ++ci ) {
			pushAll( *ci );
		}
		break;
	case BTND_PatternData_TYPE:
		AYDEBUG("pattern");
		curVec.push_back( *t.getPatternData() );
		break;
	case BTND_RewriteData_TYPE:
		AYDEBUG("this ain't a place for no rewrites");
		break;
	default:
		AYDEBUG("something is definitely broken");
	}

}



} // namespace barzer
