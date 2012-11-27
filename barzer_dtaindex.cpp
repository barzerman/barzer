#include <barzer_dtaindex.h>
//#include <boost/pool/singleton_pool.hpp>
#include <barzer_loader_xml.h>
#include <barzer_universe.h>
#include <boost/regex.hpp>

namespace barzer {

StoredEntity& StoredEntityPool::addOneEntity( bool& madeNew, const StoredEntityUniqId& uniqId )
{
	std::pair< UniqIdToEntIdMap::iterator, bool > insPair = 
		euidMap.insert( 	
			UniqIdToEntIdMap::value_type(
				uniqId, INVALID_STORED_ID
			)
		);
	madeNew = insPair.second;
	if( !madeNew ) { // this is not a new uniqId
		StoredEntityId entId =insPair.first->second;

		return storEnt.vec[ entId ];
	}

	StoredEntityId entId = storEnt.vec.size();
	(insPair.first->second) = entId; // update euid --> entid map value with the newly generated entid
	
	StoredEntity& e = storEnt.extend(); 
	e.setAll( entId, uniqId );
	addOneToEclass( uniqId.eclass );		
	return e;
}
StoredToken& StoredTokenPool::addCompoundedTok( bool& newAdded, uint32_t cwid, uint16_t numW, uint16_t len )
{
	newAdded = false;
	StoredToken* sTok = getTokByCwid( cwid );
	if( sTok ) 
		return *sTok;

	StoredTokenId tokId = storTok.vec.size();
	StoredToken& newTok = storTok.extend();
	newTok.setCompounded( tokId, cwid, numW, len );
	return newTok;
}

StoredToken& StoredTokenPool::addSingleTok( int& lang, bool& newAdded, const char* t)
{
	newAdded = false;
	StoredToken* sTok = getTokByString(t);
	if( sTok ) 
		return *sTok;
	
    size_t t_len = strlen(t);
    if( lang == LANG_UNKNOWN )
        lang = Lang::getLangNoUniverse( t, t_len );

	StoredTokenId sid = 0xffffffff;
    if( Lang::isTwoByteLang(lang) && Lang::hasTwoByteDiacritics(t,t_len,lang)) {
        std::string out; 
        Lang::twoByteStripDiacritics( out, t,t_len,lang);
	    sid = strPool->internIt( out.c_str() );
    } else 
	    sid = strPool->internIt( t );
	const char* internedStr = strPool->resolveId( sid );

	StoredTokenId tokId = storTok.vec.size();
	StoredToken& newTok = storTok.extend();
	newTok.setSingle( tokId, sid, strlen(internedStr) );
    if( Lang::hasUpperCase(t,t_len,lang) ) 
        newTok.setHasUpperCase();

	singleTokMap.insert( 
		std::make_pair(
			internedStr, tokId 
		)
	);
	newAdded = true;
	return newTok;
}

bool DtaIndex::buildEuidFromStream( StoredEntityUniqId& euid, std::istream& in) const
{
	std::string tmp;
	/// token
	if( in >> tmp ) {
        uint32_t tmpStrId = d_gp.internalString_getId( tmp.c_str() );
        if( tmpStrId == 0xffffffff )
			return false;
		euid.tokId = tmpStrId;
	}
		else return false;
	/// class
	if( in >> tmp ) {
		euid.eclass.ec = atoi(tmp.c_str());
	}
		else return false;
	/// subclass
	if( in >> tmp ) {
		euid.eclass.subclass = atoi(tmp.c_str());
	}
		else return false;
	
	return true;
}

void DtaIndex::addTokenToEntity( 
	const char* str, StoredEntity& ent, 
	const EntTokenOrderInfo& ord, const TokenEntityLinkInfo& teli, bool unique )
{
	bool newAdded= false;
	tokPool.addSingleTok( newAdded, str );
	// StoredToken& tok = tokPool.addSingleTok( newAdded, str );
	// addTokenToEntity(tok,ent,ord,teli,unique);
}

void DtaIndex::clear()
{
	tokPool.clear();
	entPool.clear();
}

DtaIndex::DtaIndex(GlobalPools& gp, ay::UniqueCharPool* sPool) :
    d_gp(gp),
	strPool(sPool),
	tokPool(strPool),
	cwTree(0),
	semFSA(0)
{
}
DtaIndex::~DtaIndex()
{
}
void DtaIndex::print( std::ostream& fp ) const
{
	fp << "Tokens: " ;
	tokPool.print(fp );
	fp << "\n";
	entPool.print(fp);
	fp << std::endl;
}

void DtaIndex::printStoredToken( std::ostream& fp, StoredTokenId id ) const
{
	fp << resolveStoredTokenStr(id) << "|";
	const StoredToken* st = tokPool.getTokByIdSafe( id );
	if( st ) 
		fp << *st ;
	else 
		fp << "UNDEFINED";
}

int DtaIndex::loadEntities_XML( const char* fileName, StoredUniverse* universe )
{
	EntityLoader_XML xmlLoader(d_gp,universe,this);
	return xmlLoader.readFile( fileName );
}

std::ostream& StoredEntityPrinter::print(const StoredEntityClass& ec ) 
{
    universe.getGlobalPools().getDtaIdx().entPool.iterateSubclass( *this, ec );
    return fp;
}
bool StoredEntityRegexFilter::operator()( const BarzerEntity& ent ) const
{
    if( entFilterMode == ENT_FILTER_MODE_ID )  { // looking for pattern in id 
        if( const char* id = universe.getGlobalPools().internalString_resolve(ent.tokId) )
            return boost::regex_search (id, pattern);
        else 
            return false;
    } else if( entFilterMode == ENT_FILTER_MODE_NAME ) { // looking for pattern in name 
        if( const EntityData::EntProp* edata = universe.getEntPropData( ent ) ) {
            return boost::regex_search (edata->canonicName, pattern);
        } else
            return false;
    } else if( entFilterMode == ENT_FILTER_MODE_BOTH ) { /// looking for pattern in either name or id
        if( const char* id = universe.getGlobalPools().internalString_resolve(ent.tokId) ) {
            if( boost::regex_search (id, pattern) ) 
                return true;
            else if( const EntityData::EntProp* edata = universe.getEntPropData( ent ) ) {
                return boost::regex_search (edata->canonicName, pattern);
            } 
        }
           
    }
    return false;
}

} // namespace barzer
