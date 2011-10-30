#include <barzer_parse.h>
#include <barzer_universe.h>
#include <barzer_el_matcher.h>
#include <barzer_barz.h>

using namespace barzer;
///////// semantic parser 

struct TopicAnalyzer {
    QSemanticParser& semParser;
    TopicAnalyzer( QSemanticParser& p ) : semParser(p) {}
    void operator() ( const NodeAndBead& nb ) {}
};

int QSemanticParser::analyzeTopics( Barz& barz, const QuestionParm& qparm  )
{
	const UniverseTrieCluster& trieCluster = universe.getTopicTrieCluster();
	const UniverseTrieCluster::BELTrieList& trieList = trieCluster.getTrieList();
    size_t grammarSeqNo = 0;

    TopicAnalyzer topicAnalyzer( *this );

    MatcherCallbackGeneric<TopicAnalyzer> cb(topicAnalyzer);

	for( UniverseTrieCluster::BELTrieList::const_iterator t = trieList.begin(); t != trieList.end(); ++t ) {
        if( *t) {
            const BELTrie& trie = *(*t);
            BarzelMatcher barzelMatcher( universe, trie );
            BarzelBeadChain& beadChain = barz.getBeads();
            barzelMatcher.get_match_greedy( cb, beadChain.lst.begin(), beadChain.lst.end(), false );
        }
        ++grammarSeqNo;
    }
    return 0;
}

int QSemanticParser::semanticize( Barz& barz, const QuestionParm& qparm  )
{
	err.clear();
	const UniverseTrieCluster& trieCluster = universe.getTrieCluster();
	const UniverseTrieCluster::BELTrieList& trieList = trieCluster.getTrieList();
    size_t grammarSeqNo = 0;
	for( UniverseTrieCluster::BELTrieList::const_iterator t = trieList.begin(); t != trieList.end(); ++t ) {
        barz.getBeads().clearUnmatchable();        

        const BELTrie* trie = *t;
		BarzelMatcher barzelMatcher( universe, *trie );
        barz.barzelTrace.setGrammarSeqNo( grammarSeqNo );
		barzelMatcher.matchAndRewrite( barz );
        ++grammarSeqNo; 
	}
	return 0;
}

/// general parser 
QParser::QParser( const StoredUniverse& u ) : 
	universe(u),
	lexer(u,&(u.getDtaIdx())),
	semanticizer(u)
{}

int QParser::interpret_only( Barz& barz, const QuestionParm& qparm )
{
	err.semRc = barz.postSemanticParse( semanticizer, qparm );
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


int QParser::parse( Barz& barz, const char* q, const QuestionParm& qparm )
{
	barz.clear();
	tokenize_only( barz, q, qparm );
	lex_only( barz, qparm );
	semanticize_only( barz, qparm );
	
	return 0;
}
