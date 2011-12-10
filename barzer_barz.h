#ifndef BARZER_BARZ_h
#define BARZER_BARZ_h

#include <barzer_el_chain.h>
#include <barzer_parse_types.h>
#include <barzer_topics.h>

namespace barzer {

class QSemanticParser;
class QLexParser;
class QTokenizer;
class StoredUniverse;

struct BarzelTrace {
    enum { 
        MAX_TAIL_REPEAT = 16  // if more than this many consecutive occurrences are detected its a loop
    }; 
    struct SingleFrameTrace {
        size_t grammarSeqNo; // grammar sequence number
        BarzelTranslationTraceInfo tranInfo; // translation trace info for the rule that was hit
        std::vector< std::string > errVec;
        
        bool eq( const SingleFrameTrace& o )  const
            { return (grammarSeqNo == o.grammarSeqNo && tranInfo.eq(o.tranInfo)); }
        bool eq( size_t gn, const BarzelTranslationTraceInfo& ti ) const 
            { return ( grammarSeqNo== gn && tranInfo.eq(ti)) ; }
        void set( size_t gn, const BarzelTranslationTraceInfo& ti ) 
            { 
                grammarSeqNo=gn;
                tranInfo = ti;
            }
        SingleFrameTrace( ) : grammarSeqNo(0) {}
        SingleFrameTrace( const BarzelTranslationTraceInfo& ti, size_t n ) : grammarSeqNo(n), tranInfo(ti) {}
    };

    typedef std::vector< SingleFrameTrace > TraceVec;
    
    size_t d_grammarSeqNo; // current grammar sequence number 

    SingleFrameTrace d_lastFrame;
    size_t d_lastFrame_count;

    TraceVec d_tvec;
    std::string skippedTriesString;

    void clear() { 
        d_tvec.clear(); 
        skippedTriesString.clear();  
        d_lastFrame_count=0;
    }
    size_t size() const { return d_tvec.size(); }
    void push_back( const BarzelTranslationTraceInfo& traceInfo ) 
    {
        d_tvec.push_back( SingleFrameTrace( traceInfo, d_grammarSeqNo ) );
        if( d_lastFrame.eq( d_grammarSeqNo, traceInfo ) ) 
            ++d_lastFrame_count;
        else {
            d_lastFrame_count= 0;
            d_lastFrame.set( d_grammarSeqNo, traceInfo );
        }
    }
    void setGrammarSeqNo( size_t gs ) { d_grammarSeqNo= gs; };

    const TraceVec& getTraceVec() const { return d_tvec; }

    BarzelTrace() : d_grammarSeqNo(0), d_lastFrame_count(0) { }

    SingleFrameTrace& getTopFrameTrace() { return d_tvec.back(); }
    const SingleFrameTrace& getTopFrameTrace() const { return d_tvec.back(); }
    
    void setError( const char* err ) 
    {
        if( d_tvec.size() ) {
            if( !d_tvec.back().errVec.size() ) 
                d_tvec.back().errVec.push_back( err );
            else 
                d_tvec.back().errVec.back().assign( err );
        }
    }
    void pushError( const char* err ) 
    {
        if( d_tvec.size() ) {
            d_tvec.back().errVec.push_back( err );
        }
    }
    bool detectLoop() const;
    bool lastFrameLoop() const { return (d_lastFrame_count>= MAX_TAIL_REPEAT); }
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
	enum { 
        MAX_TRACE_LEN = 256, 
        LONG_TRACE_LEN = 32  
    };

	BarzelTrace barzelTrace;
    BarzTopics  topicInfo;
	

    bool lastFrameLoop() const { return barzelTrace.lastFrameLoop(); }
    void pushError( const char* err ) { barzelTrace.pushError(err); }
    void setError( const char* err ) { barzelTrace.setError(err); }

    // analyzing trace to determine whether something is wrong and we're in a rewriting loop
    bool shouldTerminate() const
    {
        if( barzelTrace.size() < MAX_TRACE_LEN ) 
            return false;
        else 
            return barzelTrace.detectLoop();
    }

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

    int segregateEntities( const StoredUniverse& u, const QuestionParm& qparm, const char* q );

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
