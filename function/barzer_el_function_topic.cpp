#include <barzer_el_function.h>
#include <barzer_el_function_holder.h>
#include <barzer_datelib.h>
#include <barzer_universe.h>

namespace barzer {
using namespace funcHolder;
namespace {

    // hasTopics [number - topicThreshold ] topic1 [, ..., topicN ]
    // returns total weight of all topics in barz, whose weight is > topicThreshold
	FUNC_DECL(hasTopics)
    {
        SETFUNCNAME(hasTopic);

        int totalWeight = 0;
        
        int weightThreshold = 0;
        const BarzTopics& topicInfo = ctxt.getBarz().topicInfo;
        
		BarzelEvalResultVec::const_iterator ri = rvec.begin(); 
        if( ri != rvec.end() ) {
            const BarzerNumber* n  = getAtomicPtr<BarzerNumber>( *ri );
            if( n ) {
                weightThreshold = n->getInt();    
                ++ri;
            }
        }
		for (; ri != rvec.end(); ++ri) {
            const BarzerEntity* ent = getAtomicPtr<BarzerEntity>(*ri);
            if( ent ) {
                bool hasTopic = false;
                int weight = topicInfo.getTopicWeight( *ent, hasTopic );
                if( weight > weightThreshold )
                    totalWeight+= weight;
            }
        }

        if( totalWeight > 0 ) 
            setResult(result, BarzerNumber(totalWeight) );
        return true;
    }
    /// list, {class,subclass, filterClass, filterSubclass}[N]
FUNC_DECL(topicFilterEList) //
{
    SETFUNCNAME(topicFilterEList);

    /// first parm is the list we're filtering
    /// followed by list of pairs of class/subclass of topics we want to filter on
    /// ACHTUNG: extract the list of filter topics
    /// filter on them
    /// if this list is empty NEVER filter on own class/subclass
    ///
    BarzerEntityList outlst;
    const BarzerEntityList* entLst =0;
    BarzerEntityList tmpList;
    if( rvec.size() )  {

        entLst = getAtomicPtr<BarzerEntityList>(rvec[0]);
        if( !entLst ) {
            const BarzerEntity* ent = getAtomicPtr<BarzerEntity>(rvec[0]);
            if( ent ) {
                tmpList.addEntity( *ent );
                entLst = &tmpList;
            }
        }
    }

    if( !entLst ) {
        setResult(result, outlst);
        return true;
    }
    // at this point entLst points to the list of all entities that may belong in the outlst
    // parsing filtering topic {class,subclass, filterClass,filterSubclass} pairs
    typedef std::vector< StoredEntityClass > StoredEntityClassVec;
    typedef std::map< StoredEntityClass, StoredEntityClassVec > SECFilterMap;
    SECFilterMap fltrMap;

    if( rvec.size() > 4 )  {
        size_t rvec_size_1 = rvec.size()-3;
        for( size_t i = 1; i< rvec_size_1; i+= 4 ) {
            StoredEntityClass sec;

            {
            const BarzerNumber* cn  = getAtomicPtr<BarzerNumber>(rvec[i]);
            if( cn )
                sec.setClass( cn->getInt() );
            const BarzerNumber* scn  = getAtomicPtr<BarzerNumber>(rvec[i+1]);
            if( scn )
                sec.setSubclass( scn->getInt() );
            }
            StoredEntityClass filterSec;
            {
            const BarzerNumber* cn  = getAtomicPtr<BarzerNumber>(rvec[i+2]);
            if( cn )
                filterSec.setClass( cn->getInt() );
            const BarzerNumber* scn  = getAtomicPtr<BarzerNumber>(rvec[i+3]);
            if( scn )
                filterSec.setSubclass( scn->getInt() );
            }
            StoredEntityClassVec& filterClassVec = fltrMap[ sec ];

            if( std::find( filterClassVec.begin(), filterClassVec.end(), filterSec ) == filterClassVec.end() )
                fltrMap[ sec ].push_back( filterSec );
        }
    }
    /// here ecVec has all entity {c,sc} we're filtering on
    const BarzTopics::TopicMap& topicMap = ctxt.getBarz().topicInfo.getTopicMap();

    // computing the list of topics, currently in Barz, to filter on
    std::set< BarzerEntity > filterTopicSet;

    for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
        const BarzerEntity& topicEnt = topI->first;
        if( !fltrMap.size() )
            filterTopicSet.insert( topicEnt );
        else {
            for( SECFilterMap::const_iterator fi = fltrMap.begin(); fi != fltrMap.end(); ++fi ) {
                if( std::find( fi->second.begin(), fi->second.end(), topicEnt.eclass ) !=
                    fi->second.end())
                {
                    filterTopicSet.insert( topicEnt );
                }
            }
        }
    }

    // here filterTopicSet contains every topic we need to filter on
    if( !filterTopicSet.size() ) { /// if nothing to filter on we're short circuitin
        setResult(result, *entLst );
        return true;
    }

    // pointers to all eligible entities will be in eligibleEntVec
    std::vector< const BarzerEntity* > eligibleEntVec;

    const BarzerEntityList::EList& theEList = entLst->getList();
    eligibleEntVec.reserve( theEList.size() );
    for( BarzerEntityList::EList::const_iterator ei = theEList.begin(); ei != theEList.end(); ++ei ) {
        eligibleEntVec.push_back( &(*ei) );
    }
    // pointers to all eligible entities ARE in eligibleEntVec and all topics to filter on are in filterTopicSet

    // loopin over all entities
    typedef std::pair< BarzerEntity, StoredEntityClass > EntFilteredByPair;
    typedef std::vector< EntFilteredByPair > FilteredEntityVec;
    FilteredEntityVec fltrEntVec;

    typedef std::pair< StoredEntityClass, StoredEntityClass > EntClassPair;
    std::vector< EntClassPair > matchesToRemove;  // very small vector - things to clear (en class, filtered by class) pairs

        for( std::vector< const BarzerEntity* >::iterator eei = eligibleEntVec.begin(); eei != eligibleEntVec.end(); ++eei ) {
        const BarzerEntity* eptr = *eei;
        if( !eptr )
            continue;
        /// looping over all topics
        bool entFilterApplies = false;
        bool entPassedFilter = false;

        StoredEntityClass  filterPassedOnTopic;
        SECFilterMap::iterator entFAi;

        for( std::set< BarzerEntity >::const_iterator fi = filterTopicSet.begin(); fi != filterTopicSet.end(); ++ fi ) {
            const BarzerEntity& topicEnt = *fi;
            const StoredEntityClass& topicEntClass = topicEnt.getClass();
            // trying to filter all still eligible entities eei - eligible entity iterator
            if( eptr->eclass != topicEntClass )  {  // topic applicable for filtering if its in a different class
                // continuing to check filter applicability
                entFAi=  fltrMap.find(eptr->eclass);
                if( entFAi== fltrMap.end() )
                    continue;

                StoredEntityClassVec::iterator truncIter = std::find( entFAi->second.begin(), entFAi->second.end(), topicEntClass );
                if( truncIter  != entFAi->second.end() ) {
                    if( !entFilterApplies )
                        entFilterApplies = true;
                    const TopicEntLinkage::BarzerEntitySet  * topEntSet= q_universe.getTopicEntities( topicEnt );
                    if( topEntSet && topEntSet->find( *(eptr) ) != topEntSet->end() ) {
                        entPassedFilter = true;
                        filterPassedOnTopic  = topicEntClass;

                        if( entFAi->second.size() > 1 ) {
                            StoredEntityClassVec::iterator xx = truncIter;
                            ++xx;
                            if( xx != entFAi->second.end() ) {
                                for( StoredEntityClassVec::const_iterator x = xx; x != entFAi->second.end(); ++x ) {
                                    EntClassPair m2remove( (*eei)->eclass, *x );
                                    if( std::find( matchesToRemove.begin(), matchesToRemove.end(), m2remove ) == matchesToRemove.end() )
                                        matchesToRemove.push_back( m2remove );
                                }
                                entFAi->second.erase( xx, entFAi->second.end() );
                            }
                        }
                    }
                }
            }
        } // end of topic loop

        if( entFilterApplies ) {
            if( !entPassedFilter )
                *eei= 0;
            else { // something was eligible for filtering and passed filtering
                // we will see if there are any topics more junior than filterPassedOnTopic
                fltrEntVec.push_back( EntFilteredByPair(*(*eei), filterPassedOnTopic) );
            }
        } else { // didnt have to filter
            fltrEntVec.push_back( EntFilteredByPair(*(*eei), StoredEntityClass()) );
        }
    } // end of entity loop

    /// here all non 0 pointers in eligibleEntVec can be copied to the outresult
    /// filled the vector with all eligible

    Barz& barz = ctxt.getBarz();
    bool strictMode = barz.topicInfo.isTopicFilterMode_strict();
    for(FilteredEntityVec::const_iterator i = fltrEntVec.begin(); i!= fltrEntVec.end(); ++i ) {
        if( strictMode && !i->second.isValid() )
            continue;
        bool shouldKeep = true;
        for( std::vector< EntClassPair >::const_iterator x = matchesToRemove.begin(); x!= matchesToRemove.end(); ++x ) {
            if( x->first == i->first.eclass && x->second == i->second ) {
                /// this should be cleaned out
                shouldKeep = false;
                break;
            }
        }
        if( shouldKeep )
            outlst.addEntity( i->first );
    }
    if( !outlst.getList().size()  ) {
        for( BarzerEntityList::EList::const_iterator ei = theEList.begin(); ei != theEList.end(); ++ei ) {
            outlst.addEntity( *ei );
        }
    }

    setResult(result, outlst );
    return true;
}
//// TOPICS  2124,2353
BELFunctionStorage_holder::DeclInfo g_funcs[] = {
    FUNC_DECLINFO_INIT(hasTopics, ""),
    FUNC_DECLINFO_INIT(topicFilterEList, "")
};

} // anonymous namespace

namespace funcHolder {
void loadAllFunc_topic(BELFunctionStorage_holder* holder)
    { for( const auto& i : g_funcs ) holder->addFun( i ); }
}

} // namespace barzer
