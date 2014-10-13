#include <barzer_multiverse.h>
#include <barzer_universe.h>
#include <barzer_beni.h>
#include <ay/ay_util_time.h>
#include <ay/ay_fileiter.h>
#include <ay/ay_boost.h>

using boost::property_tree::ptree;
namespace barzer {

size_t Multiverse_BENI_Loader::loadFromFile( const std::string& path, const Multiverse_BENI_Loader::LoadParm& parm, const Multiverse_BENI_Loader::UniverseLoadDataMap& colMap )
{
    size_t recCount = 0;
    typedef std::pair<StoredUniverse*, UniverseLoadDataMap::mapped_type> UniversePtrColPair;
    std::vector<UniversePtrColPair> colLookup;

    for( const auto& x : colMap ) {
        if( StoredUniverse* u = d_gp.getUniverse(x.first) ) {
            if(!x.second.textCol.empty())
                colLookup.push_back( {u,x.second} );
            else
                std::cerr << "MULTIVERSE ERROR invalid column list for universe #" << x.first << " ... skipping \n";
        } else {
            std::cerr << "MULTIVERSE ERROR invalid universe #" << x.first << " ... skipping \n";
        }
    }
    auto tok_id = parm.col_id,
        tok_relevance = parm.col_relevance,
        tok_class= parm.col_relevance,
        tok_subclass= parm.col_subclass;

    bool overrideEc = (tok_class==0xffffffff), overrideSc = (tok_subclass==0xffffffff); // endity class/subclass override with default
    size_t reportEvery = 50000, timesReported = 0;

    std::string normName, lowerCase;
    std::vector<char> tmpBuf;
    enum {
        NAME_MODE_OVERRIDE,
        NAME_MODE_SKIP
    };
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
    AY_PAF_BEGIN(path.c_str(), parm.sep) 
        if(col.empty() || tok_id >= col.size() )
          return 0;
        int relevance = (tok_relevance <col.size()? atoi(col[tok_relevance]): 0);
        StoredEntityUniqId euid;
        const char* id = col[tok_id];

        if( !id || !*id )
          return 0;
        size_t numTokRead = 0;

        euid.eclass.ec = ( (overrideEc || !*col[tok_class]|| tok_class>= col.size()) ? parm.dfec.ec : atoi(col[tok_class]));
        euid.eclass.subclass = ( (overrideSc || !*col[tok_subclass] || tok_subclass >= col.size()) ? parm.dfec.ec : atoi(col[tok_subclass]));
        euid.tokId = d_gp.internString_internal(id);
        StoredEntity& ent = d_gp.getDtaIdx().addGenericEntity( euid.tokId, euid.eclass.ec, euid.eclass.subclass );

        for( const auto& lkup : colLookup ) {
            auto u = lkup.first;
            if( SmartBENI* smartBeni = u->beniPtr() ) {
                const auto & uniData = lkup.second;
                auto col_name = uniData.textCol.front();

                name.clear();
                if( uniData.textCol.size() > 1 ) {
                    std::stringstream sstr;
                    sstr << uniData.textCol.front();
                    for( auto i = uniData.textCol.begin()+1; i!= uniData.textCol.end(); ++i ) {
                        if( parm.nameJoinChar )
                            sstr << parm.nameJoinChar;
                        sstr << col[*i];
                    }
                    name = sstr.str();
                } else { // name is in a single column 
                    name = col[uniData.textCol.front()];
                }
                if(name == uniData.skipVal)
                  continue;
                Lang::stringToLower( tmpBuf, lowerCase, name.c_str() );
                BENI::normalize( normName, lowerCase, u );
                if( uniData.storeRelevance || uniData.storeName) {
                    if( mode == NAME_MODE_OVERRIDE )
                        u->setEntPropData( ent.getEuid(), (uniData.storeName ? name.c_str():""), relevance, true );
                    else if( mode == NAME_MODE_SKIP )
                        u->setEntPropData( ent.getEuid(), (uniData.storeName ? name.c_str():""), relevance, false );
                }
                smartBeni->beniStraight().addWord(normName, ent.getEuid() );
                if( smartBeni->hasSoundslike() )
                    smartBeni->beniSl().addWord(normName, ent.getEuid());
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
void  Multiverse_BENI_Loader::runPropertyTreeNode( const ptree& pt, const boost::optional<const ptree&> mvAttrOpt )
{
    LoadParm mvParm;
    std::string multiverseName; 
    if( mvAttrOpt ) {
        ay::ptree_opt mvattr(mvAttrOpt.get());
        if( !mvattr(mvParm.col_id, "col-id") )
            std::cerr << "MULTIVERSE WARNING: col-id attribute not found, using default value for id-col=" << mvParm.col_id << "\n";

        mvattr(multiverseName, "name");
        mvParm.dfec.ec = mvattr.get("cl",0);
        mvParm.dfec.subclass = mvattr.get("sc",0);
        mvParm.col_relevance  = mvattr.get("col-relevance", 0xffffffff);
    }
    size_t universe_count = 0;
    UniverseLoadDataMap  uColMap;
    BOOST_FOREACH(const ptree::value_type &v, pt) {
        if( v.first != "universe")
          continue;
        ++universe_count;
        if( auto attrOpt = v.second.get_child_optional("<xmlattr>") ) {
            ay::ptree_opt attr(attrOpt.get());

            uint32_t uid = 0xffffffff;
            if( !attr(uid, "uid") ) {
                std::cerr << "MULTIVERSE ERROR (" << multiverseName << "): no id specificed for universe #" << universe_count << " ... skipping \n";
                continue;
            }
            StoredUniverse& uni = d_gp.produceUniverse(uid);
            std::string uname;
            if( attr(uname, "name") )
              uni.setUserName(uname.c_str());
            uint32_t text_col = 0xffffffff;
            if( attr.get<bool>("beni-sl", false) )
                uni.setBit( UBIT_BENI_SOUNDSLIKE, true );

            if( attr(text_col, "text-col") ) {
                UniverseLoadData uniData;
                attr(uniData.storeName, "store-ent-name");
                attr(uniData.storeRelevance, "store-ent-relevance");
                attr(uniData.skipVal, "skip-val");
                auto iter = uColMap.find(uid);
                if( iter ==  uColMap.end() )
                    iter =  uColMap.insert({uid, UniverseLoadDataMap::mapped_type()}).first;

                iter->second.textCol.push_back(text_col);
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
    auto thisInstanceId = d_gp.getInstanceId();
    size_t fileCount = 0;
    const char* file_tag = "indexfile";
    BOOST_FOREACH(const ptree::value_type &v, pt) {
        if( v.first != file_tag )
             continue;
        ++fileCount;
        if( auto attrOpt = v.second.get_child_optional("<xmlattr>") ) {
            ay::ptree_opt attr(attrOpt.get());
            std::string fname;
            if( !attr(fname, "file") ) {
                std::cerr << "MULTIVERSE ERROR (" << multiverseName << ")." << file_tag << "[" << fileCount << "]: mandatory name attribute not found ... skipping\n";
                continue;
            }
            uint32_t instanceId = 0xffffffff;
            if( thisInstanceId == 0xffffffff || !attr( instanceId, "instance") || instanceId == thisInstanceId ) {
                std::cerr << "MULTIVERSE: (" << multiverseName << ")." << file_tag << "[" << fileCount << "]: loading " << fname ;
                ay::stopwatch load_timer;
                auto recCount = loadFromFile( fname, mvParm, uColMap );
                std::cerr << recCount << " records loaded in " << load_timer.calcTime() << " milliseconds " << std::endl;
            }
        }
    }
    if( !fileCount )
      std::cerr << "No indexfile tags found\n";
}


} // namespace barzer 
