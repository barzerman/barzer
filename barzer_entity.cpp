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
        StoredToken& sTok =  d_gp.getDtaIdx().addToken( fr.getTok_char( T_ENTID ) );
        uint32_t cl = fr.getTok_int(T_CLASS), sc = fr.getTok_int(T_SUBCLASS);
        const char* name = fr.getTok_char( T_NAME );
        uint32_t  relevance = fr.getTok_int( T_RELEVANCE );

        StoredEntityUniqId euid( sTok.tokId, cl, sc );
        // std::cerr << "FUCKSHIT:" << euid << ":" << &d_gp << "\n";
        d_ed.setEntPropData( euid, name, relevance ); 
        return 0;
    }

};

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

} // barzer namespace ends
