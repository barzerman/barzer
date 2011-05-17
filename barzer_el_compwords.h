#ifndef BARZER_EL_COMPWORDS_H
#define BARZER_EL_COMPWORDS_H

#include <barzer_el_trie.h>
#include <barzer_storage_types.h>
#include <boost/unordered_map.hpp> 

/// compounded words support for BARZEL 
/// compounded word has is when a few words get mapped into a special token
/// for exmaple "NEW YORK CITY" can be custom defined as such token 
/// comp 2ords are stored in the barzel trie. essentially comp wordis just an ID 
/// mapped to the trie leaf node 
namespace barzer {

class BarzelCompWordPool {
	typedef std::vector< uint32_t > AliasIdVec;
	/// CWID - compounded word id is an offset in the AliasIdVec 
	/// this vector stores aliasIds (or 0xffffffff) - ids of alias strings associated 
	/// with this compounded word (see comments below)
	AliasIdVec d_cwVec;

	/// maps alias ids back to cwid 
	typedef boost::unordered_map<uint32_t , uint32_t> AliasIdToCWIdMap;
	AliasIdToCWIdMap d_aliasMap;
	
	void addAlias( uint32_t cwid, uint32_t aliasId )
	{
		// AliasIdToCWIdMap::iterator i = d_aliasMap.find( aliasId );
		
		d_aliasMap[ aliasId ] = cwid;
	}
	
public:
	/// the value returned from this function is StoredToken::stringId for the corresponded compounded 
	/// token 
	uint32_t getCwidByAlias( uint32_t aliasId )  const
	{
		AliasIdToCWIdMap::const_iterator i = d_aliasMap.find( aliasId );
		return ( i == d_aliasMap.end() ? 0xffffffff : i->second );
	}
	bool aliasExists( uint32_t aliasId ) const { return (getCwidByAlias(aliasId) != 0xffffffff); }
	uint32_t getAliasByCwid( uint32_t cwid ) const
		{ return ( cwid < d_cwVec.size() ? d_cwVec[ cwid ] : 0xffffffff ); }

	/// we may want to define a specific alias for a particular compounded word
	/// and use it explicitly (makes for much faster loading)
	/// aliasId is a stringId say "NEW YORK CITY" --> compoundedword( "COMPID001" )
	/// during loading COMPID001 will be interned and its id used as alias for replacement
	///  default alias is 0xffffff 
	uint32_t addNewCompWordWithAlias( uint32_t aliasId = 0xffffffff ) 
	{
		if( aliasId != 0xffffffff )  {
			uint32_t cwid=  getCwidByAlias( aliasId );
			if( cwid != 0xffffffff ) 
				return cwid;
			else { 
				cwid= ( d_cwVec.push_back( aliasId ), d_cwVec.size() );
				addAlias( cwid, aliasId );
				return cwid;
			}
		} 

		return ( d_cwVec.push_back( aliasId ), d_cwVec.size() );
	}
};

}
#endif // BARZER_EL_COMPWORDS_H
