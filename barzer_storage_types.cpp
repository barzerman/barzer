#include <barzer_storage_types.h>
#include <iomanip>
namespace barzer {

void StoredToken::print( std::ostream& fp ) const
{
	 fp <<  std::ios::hex << tokId << "," <<  std::ios::hex << stringId << "," << "[" << numWords << "," << length << "]" <<
	 " ent:[" << entVec.size() << "]";
}
void StoredEntity::print( std::ostream& fp ) const
{
	fp << euid << "," <<  std::ios::hex << entId << ",relv:" << relevance <<", tok:" << tokInfoVec.size() ;
}

}


