#include <barzer_autocomplete.h>
#include <barzer_barz.h>
#include <barzer_el_matcher.h>
#include <barzer_parse.h>
#include <barzer_el_trie_walker.h>
#include <barzer_universe.h>
#include <barzer_server_response.h>

namespace barzer {

namespace {

/// g
struct AutocNodeVisotor_Callback {
    const QParser& parser;
    const StoredUniverse& universe;
    const BarzelTrieTraverser_depth * d_traverser;
    const QSemanticParser_AutocParms* d_autocParm;

    // this object will accumulate best entities by weight (up to a certain number)
    BestEntities* d_bestEnt;

    AutocNodeVisotor_Callback( const QParser& p ) :
        parser(p), universe(parser.getUniverse()) , d_traverser(0), d_bestEnt(0)
    {}
    
    void setBestEntities( BestEntities* be ) { d_bestEnt= be;  }

    void setCallbackData( const BarzelTrieTraverser_depth * t, const QSemanticParser_AutocParms* autocParm ) 
        { 
            d_traverser= t; 
            if( d_autocParm !=  autocParm ) 
                d_autocParm = autocParm;
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
                        if( se ) {  // redundant check but it's safer this way 
                            const BarzerEntity& euid = se->getEuid();
                            if( d_bestEnt ) 
                                d_bestEnt->addEntity( euid, pathLength );
                        }
                    } else { // entity list
                        uint32_t entGroupId = mkent.getEntGroupId();
                        const EntityGroup* entGrp = trie.getEntGroupById( entGroupId );

		                if( entGrp ) {
			                // building entity list
			                BarzerEntityList entList;
			                for( EntityGroup::Vec::const_iterator i = entGrp->getVec().begin(); i!= entGrp->getVec().end(); ++i ) {
				                const StoredEntity * se = universe.getDtaIdx().getEntById( *i );
                                if( se ) {
				                    const BarzerEntity& euid = se->getEuid();
                                    if( d_bestEnt ) 
                                        d_bestEnt->addEntity( euid, pathLength );
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

	BELPrintFormat fmt;

    AutocCallback( QParser& p, std::ostream& os ) :
        parser(p), fp(os) , nodeVisitorCB(0)
    {}
    const T* nodeVisitorCB_get() const { return nodeVisitorCB; }
    T* nodeVisitorCB_get() { return nodeVisitorCB; }
    void nodeVisitorCB_set( T* t ) { nodeVisitorCB=t; }


    void operator() ( const BTMIterator& bmi ) {
        const BELTrie& theTrie = bmi.getTrie();
        BELPrintContext printCtxt( theTrie, parser.getUniverse().getStringPool(), fmt );
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
    BestEntities bestEnt;
    QParser parser(d_universe);


    AutocCallback<AutocNodeVisotor_Callback> acCB(parser, d_os );

    AutocNodeVisotor_Callback nodeVisitorCB(parser);
    nodeVisitorCB.setBestEntities( &bestEnt );
    acCB.nodeVisitorCB_set( &nodeVisitorCB );
    
    MatcherCallbackGeneric< AutocCallback<AutocNodeVisotor_Callback> > cb(acCB);
	AutocStreamerJSON autocStreamer(bestEnt, d_universe );
    parser.autocomplete( cb, d_barz, q, d_qparm );
    autocStreamer.print( d_os );
    return 0;
}

} // barzer namespace ends 