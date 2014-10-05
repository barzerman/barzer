#include <barzer_multiverse.h>
#include <barzer_universe.h>

namespace barzer {

void Multiverse_BENI_Loader::addUniverseColumn( uint32_t uid, size_t col )
{
}
size_t Multiverse_BENI_Loader::loadFromFile( const Multiverse_BENI_Loader::LoadParm& parm, const Multiverse_BENI_Loader::UniverseColMap& colMap )
{
    size_t recCount = 0;

    std::vector<decltype(colMap)::value_type> colLookup;

    for( const auto& x : colMap )
        colLookup.push_back( x );

    auto tok_id = parm.col_id,
        tok_relevance = parm.col_relevance,
        tok_class= parm.col_relevance,
        tok_subclass= parm.col_subclass;

    bool overrideEc = (tok_class==0xffffffff), overrideSc = (tok_subclass==0xffffffff); // endity class/subclass override with default
    ay::stopwatch timer;
    size_t reportEvery = 50000, timesReported = 0;

    std::string normName, lowerCase;
    std::vector<char> tmpBuf;
    int mode = NAME_MODE_OVERRIDE;
    if( !parm.mode.empty() ) {
        for( const char* x = parm.mode.c_str(); *x; ++x )  {
            switch( *x ) {
            case 's': mode=NAME_MODE_SKIP; break;
            case 'c': overrideEc = overrideSc = true; break;
            case 'C': overrideEc = true; break;
            }
        }
    }
    size_t entsRead=0;
    std::string name;
    AY_PAF_BEGIN(path, parm.sep) 
        if(col.empty() || tok_id >= col.size() )
          return 0;
        int relevance = (tok_relevance <col.size()? atoi(col[tok_relevance]): 0);
        StoredEntityUniqId euid;
        const char* id = col[tok_id];

        if( !id || !*id )
          return 0;
        size_t numTokRead = 0;

        euid.eclass.ec = ( (overrideEc || !*col[tok_class]|| tok_class>= col.size()) ? dfec.ec : atoi(col[tok_class]));
        euid.eclass.subclass = ( (overrideSc || !*col[tok_subclass] || tok_subclass >= col.size()) ? dfec.ec : atoi(col[tok_subclass]));
        euid.tokId = d_gp.internString_internal(id);
        StoredEntity& ent = d_gp.getDtaIdx().addGenericEntity( euid.tokId, euid.eclass.ec, euid.eclass.subclass );

        for( const auto& lkup : colLookup ) {
            if( const StoredUniverse* u = d_gp.getUniverse(lkup.first) ) {
                auto col_name = lkup.second.front();

                name.clear();
                if( lkup.second.size() > 1 ) {
                    std::stringstream sstr;
                    sstr << lkup.front();
                    for( auto i = lkup.second.begin()+1; i!= lkup.second.end(); ++i ) {
                        if( parm.nameJoinChar )
                            sstr << parm.nameJoinChar;
                        sstr << s;
                    }
                    name = sstr.str();
                } else { // name is in a single column 
                    name = col[lkup.second.front()];
                }
                Lang::stringToLower( tmpBuf, lowerCase, name.c_str() );
                BENI::normalize( normName, lowerCase, &d_universe );
                if( mode == NAME_MODE_OVERRIDE )
                    d_universe.setEntPropData( ent.getEuid(), name.c_str(), relevance, true );
                else if( mode == NAME_MODE_SKIP )
                    d_universe.setEntPropData( ent.getEuid(), name.c_str(), relevance, false );
                d_beniStraight.addWord(normName, ent.getEuid() );
                if( d_isSL )
                    d_beniSl.addWord(normName, ent.getEuid());
            }
        }

        if (!(++entsRead % reportEvery)) {
			      std::cerr << '.';
        }
        return 0;
    AY_PAF_END
    return 0;
    return recCount;
}
namespace {

template <typename T=uint32_t>
inline bool attr_read_opt<T>( T& v, const ptree& attr, const char* attrName ) 
    { 
        if( auto x = mvAttrOpt->get_optional<T>(attrName) ) return( v = x->get(), true);
        else return true;
    }
} // end of anon namspace 
void  Multiverse_BENI_Loader::runPropertyTreeNode( const boost::property_tree::ptree& pt, const boost::optional<const ptree&> mvAttrOpt )
{
    LoadParm mvParm;
    std::string multiverseName; 
    if( mvAttrOpt ) {
        if( !attr_read_opt(mvParm.col_id, "col-id") )
            std::cerr << "MULTIVERSE WARNING: col-id attribute not found, using default value for id-col=" << mvParm.col_id << "\n";

        attr_read_opt(multiverseName, "name");
        attr_read_opt(mvParm.dfec.ec, "cl");
        attr_read_opt(mvParm.dfec.ec, "sc");
        attr_read_opt(mvParm.col_relevance, "col-relevance");
    }
    size_t universe_count = 0;
    UniverseColMap  uColMap;
    auto universePtOpt = pg.get_child_optional( "universe" );
    if( !universePtOpt ) {
        std::cerr << "No universe tags found\n";
        return;
    }
    BOOST_FOREACH(const ptree::value_type &v, universePtOpt.get()) {
        ++universe_count;
        if( auto attrOpt = v.second.get_child("<xmlattr>") ) {
            uint32_t uid = 0xffffffff;
            if( attr_read_opt(uid, "uid") ) {
                StoredUniverse& uni = d_gp.produceUniverse(uid);
            } else {
                std::cerr << "MULTIVERSE ERROR (" << multiverseName << "): no id specificed for universe #" << universe_count << " ... skipping \n";
                continue;
            }
            uint32_t text_col = 0xffffffff;
            if( attr_read_opt(text_col, "uid") ) {
                auto iter = uColMap.find(uid);
                if( iter ==  uColMap.end() ) 
                    iter =  uColMap.insert({uid, UniverseColMap::mapped_type()}).first;
                iter->second.push_back(text_col);
            } else {
                std::cerr << "MULTIVERSE ERROR (" << multiverseName << "): no text column specificed for universe #" << universe_count << " ... skipping \n";
                continue;
            }
        }
    }
    if( !uColMap.empty() ) {
        std::cerr << "MULTIVERSE ERROR (" << multiverseName << "): no valid universe tags found\n";
        return;
    }
    auto thisInstance = d_gp.getInstanceId();
    size_t fileCount = 0;
    const char* file_tag = "indexfile";
    if( auto filePtOpt = pg.get_child_optional( file_tag ) ) {
        BOOST_FOREACH(const ptree::value_type &v, filePtOpt.get()) {
            ++fileCount;
            if( auto attrOpt = v.second.get_child("<xmlattr>") ) {
                std::string fname;
                if( !attr_read_opt(fname, "file") ) {
                    std::cerr << "MULTIVERSE ERROR (" << multiverseName << ")." << file_tag << "[" << fileCount << "]: mandatory name attribute not found ... skipping\n";
                    continue;
                }
                uint32_t instanceIf = 0xffffffff;
                if( thisInstance == 0xffffffff || !attr_read_opt( instanceId, "instance") || instanceId = thisInstanceId ) {
                    std::cerr << "MULTIVERSE: (" << multiverseName << ")." << file_tag << "[" << fileCount << "]: loading " << fname ;
                    ay::stopwatch load_timer;
                    auto recCount = loadFromFile( fname, uParm, uColMap );
                    std::cerr << recCount << " records loaded in " << cmdTimer.calcTime() << " milliseconds " << std::endl;
                }
            }
        }
    } else {
        std::cerr << "No indexfile tags found\n";
    }
}


} // namespace barzer 
