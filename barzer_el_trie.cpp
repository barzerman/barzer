#include <barzer_el_trie.h>

namespace barzer {

std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp ) const
{
	return( fp << BTNDDecode::typeName_Pattern( type )  << ":" << std::hex << id );
}

} // end namespace barzer
