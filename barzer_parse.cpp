#include <barzer_parse.h>
#include <barzer_universe.h>
#include <barzer_el_matcher.h>
#include <barzer_barz.h>

using namespace barzer;
///////// semantic parser 
namespace {
struct TopicAnalyzer {
    QSemanticParser& semParser;
    Barz& barz;
    TopicAnalyzer( QSemanticParser& p, Barz& b ) : semParser(p), barz(b) {}
    void operator() ( ) {
        // std::cerr << __FILE__ << ":" << __LINE__ << " Analyzing topics\n";
        const BeadList& beadList = barz.getBeadList();
        for( BeadList::const_iterator i = beadList.begin(); i!= beadList.end(); ++i ) {
            if( i->isAtomic() ) {
                const BarzerEntity* ent = i->get<BarzerEntity>();
                if( ent ) 
                    barz.topicInfo.addTopic( *ent );
                else {
                    const BarzerEntityList* entList = i->get<BarzerEntityList>();
                    if( entList ) {
                        const BarzerEntityList::EList& el = entList->getList();
                        for( BarzerEntityList::EList::const_iterator ei = el.begin(); ei != el.end(); ++ei ) {
                            barz.topicInfo.addTopic( *ei );
                        }
                    } else {
                        const BarzerEntityRangeCombo* erc = i->get<BarzerEntityRangeCombo>();
                        if( erc ) {
                            barz.topicInfo.addTopic( erc->getEntity(), (int)(erc->getInt(BarzTopics::MIN_TOPIC_WEIGHT)) );
                        }
                    }
                }
            }
        }
        barz.topicInfo.computeTopTopics( );
    }
};
}

int QSemanticParser::analyzeTopics( Barz& barz, const QuestionParm& qparm  )
{
	const UniverseTrieCluster& trieCluster = universe.getTopicTrieCluster();
    if( !trieCluster.getTrieList().empty() ) {
        semanticize_trieList( trieCluster, barz, qparm );
        TopicAnalyzer analyzer( *this, barz );
        analyzer();
    }
    return 0;
}

int QSemanticParser::semanticize_trieList( const UniverseTrieCluster& trieCluster, Barz& barz, const QuestionParm& qparm  )
{
	const TheGrammarList& trieList = trieCluster.getTrieList();
    size_t grammarSeqNo = 0;
    std::stringstream skipsStream;
    size_t numRemainingStems = 1;
	for( TheGrammarList::const_iterator t = trieList.begin(); t != trieList.end(); ++t ) {
        // checking whether this grammar applies 
        if( ! t->grammarInfo() || t->grammarInfo()->goodToGo(barz.topicInfo) ) {
            // this handles the terminators 
            barz.getBeads().clearUnmatchable();        
            if( numRemainingStems )
                numRemainingStems = barz.getBeads().adjustStemIds(universe,t->trie());
    
            const BELTrie&  trie = t->trie();
		    BarzelMatcher barzelMatcher( universe, trie );
            barz.barzelTrace.setGrammarSeqNo( grammarSeqNo );

            if( trie.d_imperativePre.size() ) 
		        barzelMatcher.runImperatives_Pre( barz );
		    barzelMatcher.matchAndRewrite( barz );

            if( trie.d_imperativePost.size() ) 
		        barzelMatcher.runImperatives_Post( barz );
            ++grammarSeqNo; 
        } else {
            /// reporting skipped trie
            skipsStream << t->trie().getTrieClass() << ":" << t->trie().getTrieId() << "|" ;
        }
	}
    barz.barzelTrace.skippedTriesString += skipsStream.str();
    return 0;
}

bool QSemanticParser::needTopicAnalyzis() const
{
    return universe.hasTopics();
}

int QSemanticParser::parse_Autocomplete_cluster( const UniverseTrieCluster& trieCluster,MatcherCallback& cb,Barz& barz, const QuestionParm& qparm, const QSemanticParser_AutocParms& autocParm  )
{
    err.clear();
	const TheGrammarList& trieList = trieCluster.getTrieList();
    size_t grammarSeqNo = 0;
    std::stringstream skipsStream;
	for( TheGrammarList::const_iterator t = trieList.begin(); t != trieList.end(); ++t ) {
        // checking whether this grammar applies 
        if( !t->grammarInfo() || t->grammarInfo()->autocApplies() ) {
            // this handles the terminators 
            // barz.getBeads().clearUnmatchable();        
		    BarzelMatcher barzelMatcher( universe, t->trie() );
		    barzelMatcher.match_Autocomplete( cb, barz, autocParm );

            ++grammarSeqNo; 
        } else {
            /// reporting skipped trie
            skipsStream << t->trie().getTrieClass() << ":" << t->trie().getTrieId() << "|" ;
        }
	}
    barz.barzelTrace.skippedTriesString += skipsStream.str();
    return 0;
}
int QSemanticParser::parse_Autocomplete( MatcherCallback& cb,Barz& barz, const QuestionParm& qparm, const QSemanticParser_AutocParms& autocParm  )
{
    if( qparm.autoc.hasSpecificTrie() ) {
        const UniverseTrieCluster* cluster = ( qparm.autoc.needOnlyTopic()  ? 
            &(universe.getTopicTrieCluster()) : &(universe.getTrieCluster()) );

        const BELTrie* trie = cluster->getTrieByUniqueId( qparm.autoc.trieClass, qparm.autoc.trieId );
        if( trie ) {
            BarzelMatcher barzelMatcher( universe, *trie );
            barzelMatcher.match_Autocomplete( cb, barz, autocParm );
        } else {
            return 1;
        }
    } else {
        if( qparm.autoc.needTopic() ) {
            parse_Autocomplete_cluster( universe.getTopicTrieCluster(), cb, barz, qparm, autocParm );
        }
        if( qparm.autoc.needRules() ) {
            parse_Autocomplete_cluster( universe.getTrieCluster(), cb, barz, qparm, autocParm );
        }
    }
    return 0;
}

int QSemanticParser::semanticize( Barz& barz, const QuestionParm& qparm  )
{
	err.clear();
	const UniverseTrieCluster& trieCluster = universe.getTrieCluster();

	return semanticize_trieList( trieCluster, barz, qparm );
}

/// general parser 
QParser::QParser( const StoredUniverse& u ) : 
	universe(u),
    tokenizer(u),
	lexer(u,&(u.getDtaIdx())),
	semanticizer(u)
{}

int QParser::interpret_only( Barz& barz, const QuestionParm& qparm )
{
	err.semRc = barz.postSemanticParse( semanticizer, qparm );
	return 0;
}

int QParser::analyzeTopics_only( Barz& barz, const QuestionParm& qparm )
{
	err.semRc = barz.analyzeTopics( semanticizer, qparm );
	return 0;
}

int QParser::semanticize_only( Barz& barz, const QuestionParm& qparm )
{
	err.semRc = barz.semanticParse( semanticizer, qparm );
	return 0;
}

int QParser::lex_only( Barz& barz, const QuestionParm& qparm )
{
	err.lexRc = barz.classifyTokens( lexer, qparm );
	return 0;
}
int QParser::tokenize_only( Barz& barz, const char* q, const QuestionParm& qparm )
{
	err.tokRc = barz.tokenize( tokenizer, q, qparm ) ;
	return 0;
}


int QParser::lex_strat_advanced( Barz& barz, const char* q, const QuestionParm& qparm )
{
	barz.clear();
    barz.classifyTokens( universe.getTokenizerStrategy(), tokenizer, lexer, q, qparm ) ;
    return 0;
}
int QParser::lex_strat_default( Barz& barz, const char* q, const QuestionParm& qparm )
{
	barz.clear();
    tokenize_only( barz, q, qparm );
    lex_only( barz, qparm );
    barz.chainInit(qparm);
    return 0;
}

int QParser::lex( Barz& barz, const char* q, const QuestionParm& qparm )
{
    if( universe.getTokenizerStrategy().isDefault() ) {
        return lex_strat_default(barz,q,qparm);
    } else {
        return lex_strat_advanced(barz,q,qparm);
    }
    return 0;
}
int QParser::autocomplete( MatcherCallback& cb, Barz& barz, const char* q, const QuestionParm& qparm )
{
    barz.clear();
    tokenize_only(barz,q,qparm);

	err.lexRc = barz.classifyTokens( lexer, qparm );
    
    barz.parse_Autocomplete( cb, semanticizer, qparm  );

    return 0;
}

int QParser::barz_parse( Barz& barz, const QuestionParm& qparm )
{
    barz.analyzeTopics( semanticizer, qparm, false );
	barz.semanticParse( semanticizer, qparm, false );
    
    if( universe.needEntitySegregation() ) 
        barz.segregateEntities( universe, qparm, 0 );
    if( !universe.checkBit(StoredUniverse::UBIT_NO_ENTRELEVANCE_SORT))  
        barz.sortEntitiesByRelevance( universe, qparm, 0 );
    
	return 0;
}

int QParser::parse( Barz& barz, const char* q, const QuestionParm& qparm )
{
	barz.clear();

    if( universe.getTokenizerStrategy().isDefault() ) {
	    tokenize_only( barz, q, qparm );
	    lex_only( barz, qparm );
    } else {
        barz.classifyTokens( universe.getTokenizerStrategy(), tokenizer, lexer, q, qparm ) ;
	    // lex_only( barz, qparm );
        // lex_strat_advanced(barz,q,qparm);

    }

	semanticize_only( barz, qparm );
    
    if( universe.needEntitySegregation() ) 
        barz.segregateEntities( universe, qparm, q );
    if( !universe.checkBit(StoredUniverse::UBIT_NO_ENTRELEVANCE_SORT))  
        barz.sortEntitiesByRelevance( universe, qparm, q );
    
	return 0;
}
