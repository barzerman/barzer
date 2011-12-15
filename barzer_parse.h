#ifndef BARZER_PARSE_H
#define BARZER_PARSE_H
#include <ay/ay_headers.h>
#include <barzer_lexer.h>
#include <barzer_el_matcher.h>

namespace barzer {
class StoredUniverse;

class UniverseTrieCluster;

class QSemanticParser {
protected:
	const StoredUniverse& universe;
	// BarzelMatcher barzelMatcher;
public:
    bool needTopicAnalyzis() const ;
	struct Error : public QPError { } err;

    int semanticize_trieList( const UniverseTrieCluster& trieCluster, Barz& barz, const QuestionParm& qparm  );
	virtual int parse_Autocomplete( MatcherCallback& cb, Barz&, const QuestionParm&  );
	virtual int semanticize( Barz&, const QuestionParm&  );
	virtual int analyzeTopics( Barz&, const QuestionParm&  );
	QSemanticParser( const StoredUniverse& u) : 
		universe(u)
	{}
	virtual ~QSemanticParser() {}
    const StoredUniverse& getUniverse() const { return universe; }
};

/// invokes tokenizer, lex parser and semantical parser 
/// to produce a valid Barze
class QParser {
protected:
	const StoredUniverse & universe;
	struct Error : public QPError { 
		int tokRc; // tokenizer rcode 
		int lexRc; // lexer rcode 
		int semRc; // semantic rcode 

		void clear() { 
			tokRc=lexRc=semRc=0;
			Error::clear();
		}
		Error() : tokRc(0), lexRc(0), semRc(0) {}
	} err;
public:
	QTokenizer tokenizer;
	QLexParser lexer;
	QSemanticParser semanticizer;
	
	QParser( const StoredUniverse& u );

	/// wipes out higher ctokens/punits and tokenizes q
	int tokenize_only( Barz& barz, const char* q, const QuestionParm& qparm );

	/// wipes out beads and repopulates ctokens
	int lex_only( Barz& barz, const QuestionParm& qparm );

	/// analyzes ctokens and repopulates beads
	int semanticize_only( Barz& barz, const QuestionParm& qparm );
	int analyzeTopics_only( Barz& barz, const QuestionParm& qparm );

	/// post-semantical processing of beads by exercising a few heuristics:
	/// - groups consecutive entities of the same type  
	int interpret_only( Barz& barz, const QuestionParm& qparm );

	 
	/// tokenizes, classifies and semanticizes 
	int autocomplete( MatcherCallback& cb, Barz& barz, const char* q, const QuestionParm& qparm );
	int parse( Barz& barz, const char* q, const QuestionParm& qparm );

    // tokenizes and lexes (no semantics) 
	int lex( Barz& barz, const char* q, const QuestionParm& qparm );
    
    const StoredUniverse& getUniverse() const { return universe; }
};

} // barzer namespace ends 

#endif // BARZER_PARSE_H
