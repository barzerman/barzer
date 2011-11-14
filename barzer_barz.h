#ifndef BARZER_BARZ_h
#define BARZER_BARZ_h

#include <barzer_el_chain.h>
#include <barzer_parse_types.h>
#include <barzer_topics.h>

namespace barzer {

class QSemanticParser;
class QLexParser;
class QTokenizer;

struct BarzelTrace {
    struct SingleFrameTrace {
        size_t grammarSeqNo; // grammar sequence number
        BarzelTranslationTraceInfo tranInfo; // translation trace info for the rule that was hit
        std::vector< std::string > errVec;
        
        SingleFrameTrace( ) : grammarSeqNo(0) {}
        SingleFrameTrace( const BarzelTranslationTraceInfo& ti, size_t n ) : grammarSeqNo(n), tranInfo(ti) {}
    };

    typedef std::vector< SingleFrameTrace > TraceVec;
    size_t d_grammarSeqNo; // current grammar sequence number 
    TraceVec d_tvec;
    std::string skippedTriesString;

    void clear() { d_tvec.clear(); skippedTriesString.clear();  }
    size_t size() const { return d_tvec.size(); }
    void push_back( const BarzelTranslationTraceInfo& traceInfo ) 
    {
        d_tvec.push_back( SingleFrameTrace( traceInfo, d_grammarSeqNo ) );
    }
    void setGrammarSeqNo( size_t gs ) { d_grammarSeqNo= gs; };

    const TraceVec& getTraceVec() const { return d_tvec; }

    BarzelTrace() : d_grammarSeqNo(0) { }

    SingleFrameTrace& getTopFrameTrace() { return d_tvec.back(); }
    const SingleFrameTrace& getTopFrameTrace() const { return d_tvec.back(); }
    
    void pushError( const char* err ) 
    {
        if( d_tvec.size() ) {
            d_tvec.back().errVec.push_back( err );
        }
    }
}; 

// collection of punits and the original question
class Barz {
	/// original question with 0-s terminating tokens
	/// all poistional info, pointers and offsets from everything
	/// contained in puVec refers to this string. 
	/// so this string is almost always *longer* than the input question
	std::vector<char> question; 
	/// exact copy of the original question
	std::string questionOrig; 

	
	TTWPVec ttVec; 
	CTWPVec ctVec; 
	BarzelBeadChain beadChain;	

	friend class QSemanticParser;
	friend class QLexParser;
	friend class QTokenizer;

	friend class QParser;
	
	/// called from tokenize, ensures that question has 0-terminated
	/// tokens and that ttVec tokens point into it
	void syncQuestionFromTokens();
public:
	enum { MAX_TRACE_LEN = 256 };

	BarzelTrace barzelTrace;
    BarzTopics  topicInfo;
	
	void pushTrace( const BarzelTranslationTraceInfo& trace ) { 
		if( barzelTrace.size() < MAX_TRACE_LEN ) 
			barzelTrace.push_back( trace ) ; 
	}

	const TTWPVec& getTtVec() const { return  ttVec; }
	const CTWPVec& getCtVec() const { return ctVec; }

	BarzelBeadChain& getBeads() { return beadChain; }
	const BarzelBeadChain& getBeads() const { return beadChain; }

	void clear();
	void clearWithTraceAndTopics();

	void clearBeads();

	int tokenize( QTokenizer& , const char* q, const QuestionParm& );
	int classifyTokens( QLexParser& , const QuestionParm& );
	int chainInit( const QuestionParm& );
	int semanticParse( QSemanticParser&, const QuestionParm& );
	int analyzeTopics( QSemanticParser&, const QuestionParm& );
	int postSemanticParse( QSemanticParser&, const QuestionParm& );

	/// returns pair. first is the number of units which have been modified semantically
	/// mening - fluff, date, entity, erc, expression etc
	// second - number of beads in the barz 
	std::pair<size_t,size_t> getBeadCount() const
	{
		std::pair< size_t, size_t > rv(0,0);
		for( BeadList::const_iterator i = beadChain.getLstBegin(); i!= beadChain.getLstEnd(); ++i ) {
			++(rv.second);
			if( i->isSemantical() ) ++(rv.first);
		}
		return rv;
	}
    const BeadList& getBeadList() const { return beadChain.lst; }
};
}
#endif // BARZER_BARZ_h
