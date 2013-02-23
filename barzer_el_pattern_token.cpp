/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_el_pattern_token.h>
#include <barzer_el_trie.h>
namespace barzer {

std::ostream&  BTND_Pattern_Punct::print( std::ostream& fp, const BELPrintContext&  ) const
{
	return ( fp << "Punct (" << (char)theChar << ")" );
}

std::ostream&  BTND_Pattern_CompoundedWord::print( std::ostream& fp , const BELPrintContext& ) const
{
	return ( fp << "compword(" << std::hex << compWordId << ")");
}
std::ostream&  BTND_Pattern_Token::print( std::ostream& fp , const BELPrintContext& ctxt ) const
{
	return ( fp << "token[" << ctxt.printableString( stringId ) << "]");
}

}// namespace barzer
