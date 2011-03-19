#include <barzer_storage_types.h>
#include <iomanip>
namespace barzer {

void StoredToken::print( std::ostream& fp ) const
{
	 fp << theTok << ",tokid:" << tokId << ", strid:" <<  stringId << "," << "[" << numWords << "," << length << "]" <<
	 " ent:[" << entVec.size() << "]";
}
void StoredEntity::print( std::ostream& fp ) const
{
	fp << "euid:" << euid << ", entid:" <<  std::ios::hex << entId << ",relv:" << relevance <<", tok:" << tokInfoVec.size() ;
}

}


