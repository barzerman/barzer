#include <boost/shared_ptr.hpp>
#include <barzer_autocomplete.h>
#include <barzer_barz.h>
#include <barzer_el_matcher.h>
#include <barzer_parse.h>
#include <barzer_el_trie_walker.h>
#include <barzer_universe.h>
#include <barzer_server_response.h>
#include <barzer_geoindex.h>

namespace barzer {

namespace {
	
class AutocFilter
{
public:
	virtual ~AutocFilter() {}
	
	virtual bool acceptEntity(const StoredEntity*) const = 0;
};

class ProximityFilter : public AutocFilter
{
	const FilterParams m_params;
	const StoredUniverse& m_uni;
public:
	ProximityFilter(const FilterParams& params, const StoredUniverse& uni)
	: m_params(params)
	, m_uni(uni)
	{}
	
	bool acceptEntity (const StoredEntity *se) const
	{
		if (!se)
			return false;
		
		const auto id = se->entId;
		if (!m_params.m_subclasses.empty() &&
				std::find(m_params.m_subclasses.begin(), m_params.m_subclasses.end(), se->getSubclass()) == m_params.m_subclasses.end())
			return true;
		
		return m_uni.getGeo()->proximityFilter(id, m_params.m_point, m_params.m_dist);
	}
};

/// g
struct AutocNodeVisotor_Callback {
    const Barz& barz;
    const QParser& parser;
    const StoredUniverse& universe;
    const BarzelTrieTraverser_depth * d_traverser;
    const QSemanticParser_AutocParms* d_autocParm;
	const AutocFilter* d_filter;

    // this object will accumulate best entities by weight (up to a certain number)
    BestEntities* d_bestEnt;
    // const GlobalPools& gp;
    const QuestionParm& d_qparm;

    AutocNodeVisotor_Callback( const Barz& b, const QParser& p, const QuestionParm& qparm ) :
        barz(b), parser(p), universe(p.getUniverse()) , d_traverser(0), d_autocParm(0), d_filter(0), d_bestEnt(0), d_qparm(qparm)
    {}

    void setFilter(const AutocFilter *filter) { d_filter = filter; }

    void setBestEntities( BestEntities* be ) { d_bestEnt= be;  }

    void setCallbackData( const BarzelTrieTraverser_depth * t, const QSemanticParser_AutocParms* autocParm ) 
        { 
            d_traverser= t; 
            if( d_autocParm !=  autocParm ) 
                d_autocParm = autocParm;
        }

    bool entOkWithTopics( const StoredEntityUniqId& euid ) {
        if( barz.topicInfo.hasTopics() ) {
            const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
            for( BarzTopics::TopicMap::const_iterator ti = topicMap.begin(); ti != topicMap.end(); ++ti ) {
                const TopicEntLinkage::BarzerEntitySet* topEntSet= universe.getTopicEntities( ti->first );
        
                if( topEntSet && topEntSet->find(euid)!= topEntSet->end() ) 
                    return true;
            }
            return false;
        } else 
            return true;
    }
    bool operator()( const BarzelTrieNode& tn )
    {
        if( !d_traverser )
            return true;
        const BELTrie& trie = d_traverser->getTrie();
        
        const BarzelTranslation* translation = trie.getBarzelTranslation(tn);

        if( translation ) {
            size_t pathLength = ( d_traverser->getStackDepth()*100 + (d_autocParm? d_autocParm->lastTokTailLength:0));

            // std::cerr << "leaf " << &tn << ":";
            BTND_RewriteData rwrData; 
            translation->fillRewriteData(rwrData);

            if( rwrData.which() == BTND_Rewrite_MkEnt_TYPE ) {
                BTND_Rewrite_MkEnt mkent = boost::get<BTND_Rewrite_MkEnt>(rwrData);
                if( mkent.isValid() ) {
                    if( mkent.isSingleEnt() ) { // single entity 
                        uint32_t entId = mkent.getEntId();
                        const StoredEntity * se = universe.getDtaIdx().getEntById( entId );
                        if( se && (!d_filter || d_filter->acceptEntity(se))) {
                            const BarzerEntity& euid = se->getEuid();

                            if( d_bestEnt ) {
                                const EntityData::EntProp* edata = universe.gp.entData.getEntPropData( euid );
                                uint32_t relevance;
                                // std::cerr << "SHITFUCK:" << euid << ":" << &(universe.gp) << "\n";
                                if( edata ) 
                                    relevance = edata->relevance;
                                else 
                                    relevance = 0;

                                if( d_qparm.autoc.entFilter(euid) ) {
                                    if( barz.topicInfo.hasTopics() ) {
                                        if( entOkWithTopics(euid) )
                                            d_bestEnt->addEntity( euid, pathLength,relevance );
                                    } else
                                        d_bestEnt->addEntity( euid, pathLength,relevance );
                                }
                            }
                        }
                    } else { // entity list
                        uint32_t entGroupId = mkent.getEntGroupId();
                        const EntityGroup* entGrp = trie.getEntGroupById( entGroupId );

		                if( entGrp ) {
			                // building entity list
			                BarzerEntityList entList;
			                for( EntityGroup::Vec::const_iterator i = entGrp->getVec().begin(); i!= entGrp->getVec().end(); ++i ) {
				                const StoredEntity * se = universe.getDtaIdx().getEntById( *i );
                                if( se && (!d_filter || d_filter->acceptEntity(se)) ) {
				                    const BarzerEntity& euid = se->getEuid();
                                    if( d_bestEnt ) {
                                        const EntityData::EntProp* edata = universe.gp.entData.getEntPropData( euid );
                                        uint32_t relevance ;
                                        // std::cerr << "SHITFUCK 87:" << euid << "\n";
                                        if( edata ) {
                                            relevance = edata->relevance;
                                        } else 
                                            relevance = 0;
                                        if( d_qparm.autoc.entFilter(euid) ) {
                                            if( barz.topicInfo.hasTopics() ) {
                                                if( entOkWithTopics(euid) )
                                                    d_bestEnt->addEntity( euid, pathLength,relevance );
                                            } else
                                                d_bestEnt->addEntity( euid, pathLength,relevance );
                                        }
                                    }
                                }
			                }
		                }
                    }
                }  else {
                    // invalid entity 
                }
            } else{
                // std::cerr << "NON ENTITY";
            }
            // std::cerr << "\n";
        } else {
            // std::cerr << ".. NON-LEAF NODE\n";
        }
        return true;
    }
};

template <typename T>
struct AutocCallback {
    QParser& parser;
    std::ostream& fp;
    
    T* nodeVisitorCB;

	// BELPrintFormat fmt;

    AutocCallback( QParser& p, std::ostream& os ) :
        parser(p), fp(os) , nodeVisitorCB(0)
    {}
    const T* nodeVisitorCB_get() const { return nodeVisitorCB; }
    T* nodeVisitorCB_get() { return nodeVisitorCB; }
    void nodeVisitorCB_set( T* t ) { nodeVisitorCB=t; }


    void operator() ( const BTMIterator& bmi ) {
        const BELTrie& theTrie = bmi.getTrie();
        // BELPrintContext printCtxt( theTrie, parser.getUniverse().getStringPool(), fmt );
        const NodeAndBeadVec& nb = bmi.getMatchPath();
        if( nb.size() ) {
            const BarzelTrieNode* lastNode = nb.back().first;
            if( nodeVisitorCB ) {
                BarzelTrieTraverser_depth trav( theTrie );
                nodeVisitorCB->setCallbackData( &trav, bmi.getAutocParm() );
                (*nodeVisitorCB)( *lastNode );
    
                trav.traverse( *nodeVisitorCB, *lastNode );
            }
        }
    }
};
} /// end of anon space 
int BarzerAutocomplete::parse( const char* q )
{
    BestEntities bestEnt(d_qparm.autoc.numResults);
    QParser parser(d_universe);
	
	boost::shared_ptr<AutocFilter> filterPtr;
	if( d_barz.hasVarNotEqualTo("geo::enableFilter","false") ) {
        const auto& params = FilterParams::fromBarz(d_barz);
        if (params.m_valid)
            filterPtr.reset(new ProximityFilter(params, d_universe));
	}

    AutocCallback<AutocNodeVisotor_Callback> acCB(parser, d_os );

    AutocNodeVisotor_Callback nodeVisitorCB(d_barz,parser,d_qparm);
	nodeVisitorCB.setFilter(filterPtr.get());
    nodeVisitorCB.setBestEntities( &bestEnt );
    acCB.nodeVisitorCB_set( &nodeVisitorCB );
    
    MatcherCallbackGeneric< AutocCallback<AutocNodeVisotor_Callback> > cb(acCB);
	AutocStreamerJSON autocStreamer(bestEnt, d_universe );
    parser.autocomplete( cb, d_barz, q, d_qparm );
    autocStreamer.print( d_os );
    return 0;
}

} // barzer namespace ends 
