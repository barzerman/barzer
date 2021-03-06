
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_storage_types.h>
#include <iomanip>
namespace barzer {

void StoredToken::print( std::ostream& fp ) const
{
	 fp << "tokid:" << tokId << ", strid:" <<  
	 stringId << ",[" << numWords << ',' << length << ']'; 
	 /* rudiment from the times entities and tokens were cross linked
	 " ent:" << entVec.size() << '[' ;
	 for( SETELI_pair_vec::const_iterator i = entVec.begin(); i!= entVec.end(); ++i )  {
	 	fp << i->first << ',';
	 }

	 fp << ']';
	 */
}
void StoredEntity::print( std::ostream& fp ) const
{
	fp << "euid:" << euid << ", entid:" <<  entId << ",relv:" << relevance ;
	/*
	<<", tok:" << tokInfoVec.size() << '[';
	for( STSI_vec::const_iterator i = tokInfoVec.begin(); i!= tokInfoVec.end(); ++i ) {
		fp << i->first << ',';
	}
	fp << ']';
	*/
}

std::ostream& BarzerEntityList::print( std::ostream& fp ) const
{
	for( EList::const_iterator i = d_lst.begin(); i!= d_lst.end(); ++i ) {
		fp << '{' << *i << "}";
	}
	return fp;
}
}


