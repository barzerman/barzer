
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_storage_types.h>
#include <iomanip>
#include <barzer_entity.h>
namespace barzer {

void StoredToken::print( std::ostream& fp ) const
{
	 fp << "tokid:" << tokId << ", strid:" <<  
	 stringId << ",[" << numWords << ',' << length << ']'; 
}
void StoredEntity::print( std::ostream& fp ) const
    { fp << "euid:" << euid << ", entid:" <<  entId << ",relv:" << relevance ; }

std::ostream& BarzerEntityList::print( std::ostream& fp ) const
{
	for( EList::const_iterator i = d_lst.begin(); i!= d_lst.end(); ++i ) {
		fp << '{' << *i << "}";
	}
	return fp;
}
}
