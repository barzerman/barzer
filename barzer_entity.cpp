/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_entity.h>
#include <ay/ay_util.h>
#include <barzer_universe.h>

namespace barzer {

namespace {

struct EntityDataReaderCB {

    EntityData& d_ed;
    GlobalPools& d_gp;
    
    EntityDataReaderCB( GlobalPools& gp, EntityData& ed ) : d_ed(ed), d_gp(gp) {}
    enum { T_CLASS, T_SUBCLASS, T_ENTID, T_RELEVANCE, T_NAME };

    int operator()( ay::FileReader& fr ) {
        // const std::vector< const char* >& tok = fr.tok();
        uint32_t strId =  d_gp.internString_internal( fr.getTok_char( T_ENTID ) );
        uint32_t cl = fr.getTok_int(T_CLASS), sc = fr.getTok_int(T_SUBCLASS);
        const char* name = fr.getTok_char( T_NAME );
        uint32_t  relevance = fr.getTok_int( T_RELEVANCE );

        StoredEntityUniqId euid( strId, cl, sc );
        d_ed.setEntPropData( euid, name, relevance ); 
        return 0;
    }

};

}
EntityData::EntProp*  EntityData::setEntPropData( const StoredEntityUniqId& euid, const char* name, uint32_t rel, bool overrideName )
{
    EntPropDtaMap::iterator i = d_autocDtaMap.insert( EntPropDtaMap::value_type(euid,EntProp()) ).first;
    /// longer name always wins
    if( name && (overrideName ||(strlen(name)> i->second.canonicName.length()) )) {
        i->second.canonicName.assign(name);
    }
    if( rel )
        i->second.relevance = rel;
    return ( &(i->second) );
}
size_t EntityData::readFromFile( GlobalPools& gp, const char* fname ) 
{
    if( !fname ) {
        AYLOG(ERROR) << "invalid file\n";
        return 0;
    }
    EntityDataReaderCB cb( gp, *this );
    ay::FileReader fr;
    size_t numRec =  fr.readFile( cb, fname );
    if( !numRec ) {
        if( !fr.getFile() ) {
            AYLOG(ERROR) << "could not open file " << fname << " to read entity properties\n";
        }
    }
    return numRec;
}
void BarzerEntity::print( std::ostream& fp, const StoredUniverse& universe, int fmt ) const
{
    const char* id = universe.getGlobalPools().internalString_resolve(tokId);
    if( fmt == ENT_PRINT_FMT_PIPE ) {
        fp << eclass.ec << '|' << eclass.subclass << '|' ;
        if( id ) 
            xmlEscape( id, fp );
    } else { // ENT_PRINT_FMT_XML
        fp << "<entity class=\"" << eclass.ec << "\" esubclass=\"" << eclass.subclass << "\"";
        if( id ) 
            xmlEscape( id, fp<< " id=\"" ) << "\"";
        if( const EntityData::EntProp* edata = universe.getEntPropData( *this ) )
            xmlEscape( edata->canonicName.c_str(), fp << "n=\"" ) << "\" ";

        fp << "/>";
    }
}

void BestEntities::addEntity( const StoredEntityUniqId& euid, uint32_t pathLen, uint32_t relevance)
{
    EntIdMap::iterator i = d_entMap.find( euid );

    BestEntities_EntWeight wght( pathLen, relevance );

    /// if the set is full
    if( d_weightMap.size() >= d_maxEnt ) { 
        // check that wght is better than the worst weight in the set 
        EntWeightMap::reverse_iterator revI = d_weightMap.rbegin();
        if( !(wght < revI->first) ) 
            return;
    }
    if( i == d_entMap.end() ) { // this entity is not stored 
        EntWeightMap::iterator wi = d_weightMap.insert( EntWeightMap::value_type(wght, euid) );
        d_entMap[ euid ] = wi;
    } else {  // entity is already here
        if( wght < i->second->first )  { // if this occurence has higher weight 
            d_weightMap.erase( i->second );
            EntWeightMap::iterator wi = d_weightMap.insert( EntWeightMap::value_type(wght, euid) );
            i->second = wi;
        } else { // else we skip it
            return;
        }
        
    }
    if( d_weightMap.size() > d_maxEnt ) {
        EntWeightMap::iterator fwdIter = d_weightMap.end();
        --fwdIter;
        EntIdMap::iterator idMapIter = d_entMap.find( fwdIter->second );
        if( idMapIter->second != fwdIter ) { // should never happen
            AYLOG(ERROR) << "BestEntities inconsistency";
        } else {
            d_entMap.erase(idMapIter);
            d_weightMap.erase(fwdIter);
        }
    }
}
} // barzer namespace ends
