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


} // namespace barzer
