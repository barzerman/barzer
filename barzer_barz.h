#ifndef BARZER_BARZ_h
#define BARZER_BARZ_h

#include <barzer_el_chain.h>
#include <barzer_parse_types.h>
#include <barzer_topics.h>
#include <barzer_tokenizer.h>
#include <ay/ay_utf8.h>

namespace barzer {

class QSemanticParser;
class QLexParser;
class QTokenizer;
class StoredUniverse;
struct MatcherCallback;

struct RequestVariableMap;
struct RequestEnvironment; // server request environment

struct BarzelTrace {
    enum { 
        MAX_TAIL_REPEAT = 16  // if more than this many consecutive occurrences are detected its a loop
    }; 
    struct SingleFrameTrace {
        size_t grammarSeqNo; // grammar sequence number
        BarzelTranslationTraceInfo tranInfo; // translation trace info for the rule that was hit
        uint32_t globalTriePoolId; /// global trie pool id of the trie that was hit 
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
        SingleFrameTrace( ) : grammarSeqNo(0),globalTriePoolId(0xffffffff) {}
        SingleFrameTrace( const BarzelTranslationTraceInfo& ti, size_t n, uint32_t trieId ) : grammarSeqNo(n), tranInfo(ti),globalTriePoolId(trieId) {}
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
    void push_back( const BarzelTranslationTraceInfo& traceInfo, uint32_t trieId ) 
    {
        d_tvec.push_back( SingleFrameTrace( traceInfo, d_grammarSeqNo, trieId ) );
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
    // max number of errors to report per trace
    enum { BARZEL_TRACE_MAX_ERR=8 };
    void pushError( const char* err ) 
    {
        if( d_tvec.size() ) {
            if( d_tvec.back().errVec.size() < BARZEL_TRACE_MAX_ERR )  
                d_tvec.back().errVec.push_back( err );
            else if( d_tvec.back().errVec.back() != "<error>Too many errors to report...</error>" ) {
                d_tvec.back().errVec.push_back( "<error>Too many errors to report...</error>" );
            }
        }
    }
    bool detectLoop() const;
    bool lastFrameLoop() const { return (d_lastFrame_count>= MAX_TAIL_REPEAT); }
}; 

/// hints store information which may affect classification strategy (for instance russian classification woiuld treat ',' as a decimal separator 
class BarzHints {
public:
	enum HintsFlag
	{
		BHB_DECIMAL_COMMA,
		// add above
		BHB_MAX
	};
    /// ordered array of utf8 languages the engine will try stemming to 
    // ay:: langs
    typedef std::vector< int > LangArray;

private:
    LangArray d_stemUtf8Lang;
    bool      d_hasAsciiLang; /// true if universe has ascii language

	ay::bitflags<BHB_MAX> d_bhb;
	const StoredUniverse* d_universe;
public:

	void initFromUniverse( const StoredUniverse* u );
	void clear() { d_bhb.clear(); }

	BarzHints( ) : d_hasAsciiLang(true),d_universe(0) {}
	BarzHints( const StoredUniverse* u ) : d_universe(0)
		{ initFromUniverse(u); }

	void setHint(HintsFlag, bool val = true);
	void clearHint(HintsFlag);
	bool testHint(HintsFlag) const;

    const LangArray& getUtf8Languages() const { return d_stemUtf8Lang; }
    void  addUtf8Language( int lang ) { d_stemUtf8Lang.push_back(lang); }
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
    ay::StrUTF8 questionOrigUTF8; /// our utf8 representation of questionOrig

    /// this is a user-supplied id of the question (optional)
    uint64_t    d_origQuestionId; 
    
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

	BarzHints m_hints;
    RequestEnvironment* d_serverReqEnv;
public:
	enum { 
        MAX_TRACE_LEN = 256, 
        LONG_TRACE_LEN = 32  
    };

	BarzelTrace barzelTrace;
    BarzTopics  topicInfo;
    
    Barz() : 
        d_origQuestionId(std::numeric_limits<uint64_t>::max()),
        d_serverReqEnv(0)
    {}

    void setServerReqEnv( RequestEnvironment* env ) { d_serverReqEnv = env; }
    RequestEnvironment* getServerReqEnv() { return d_serverReqEnv; }
    const RequestEnvironment* getServerReqEnv() const { return d_serverReqEnv; }


    /// working with request variables - these variables reside in the top level request context. they live through the whole request
    /// and aren't tied to any one rewrite
    const RequestVariableMap* getRequestVariableMap()  const ;
    RequestVariableMap* getRequestVariableMap() ;

    bool getReqVarValue( BarzelBeadAtomic_var& v, const char* n ) const;
    const BarzelBeadAtomic_var*  getReqVarValue( const char* n ) const;
    void setReqVarValue( const char* n, const BarzelBeadAtomic_var& v );
    bool unsetReqVar( const char* n );
    //// end of request variable context functions 

	void setUniverse(const StoredUniverse*);

	const BarzHints& getHints() const;

    bool lastFrameLoop() const { return barzelTrace.lastFrameLoop(); }
    void pushError( const char* err ) { barzelTrace.pushError(err); }
    void setError( const char* err ) { barzelTrace.setError(err); }
    
    bool hasProperties() const { return topicInfo.hasProperties(); }
    bool hasZeroUniverseProperties() const { return topicInfo.hasZeroUniverseProperties(); }

    // analyzing trace to determine whether something is wrong and we're in a rewriting loop
    bool shouldTerminate() const
    {
        if( barzelTrace.size() < MAX_TRACE_LEN ) 
            return false;
        else 
            return barzelTrace.detectLoop();
    }

	void pushTrace( const BarzelTranslationTraceInfo& trace, uint32_t trieId ) { 
		if( barzelTrace.size() < MAX_TRACE_LEN ) 
			barzelTrace.push_back( trace, trieId ) ; 
	}

	TTWPVec& getTtVec() { return  ttVec; }
	CTWPVec& getCtVec() { return ctVec; }
	const TTWPVec& getTtVec() const { return  ttVec; }
	const CTWPVec& getCtVec() const { return ctVec; }

	BarzelBeadChain& getBeads() { return beadChain; }
	const BarzelBeadChain& getBeads() const { return beadChain; }

	void clear();
	void clearWithTraceAndTopics();

	void clearBeads();

	int tokenize( QTokenizer& , const char* q, const QuestionParm& );
	int tokenize( const TokenizerStrategy& strat, QTokenizer& tokenizer, const char* q, const QuestionParm& qparm );

    //// advanced classification method
	int classifyTokens( const TokenizerStrategy&, QTokenizer& , QLexParser&, const char* q, const QuestionParm& );

	int classifyTokens( QLexParser& , const QuestionParm& );
	int chainInit( const QuestionParm& );
	int parse_Autocomplete( QSemanticParser&, const QuestionParm& );

	int parse_Autocomplete( MatcherCallback& cb, QSemanticParser&, const QuestionParm& );
	int semanticParse( QSemanticParser&, const QuestionParm&, bool needInit=true );
	int analyzeTopics( QSemanticParser&, const QuestionParm&, bool needInit=true );

	int postSemanticParse( QSemanticParser&, const QuestionParm& );

    int segregateEntities( const StoredUniverse& u, const QuestionParm& qparm, const char* q );
    int sortEntitiesByRelevance( const StoredUniverse& u, const QuestionParm& qparm, const char* q );

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
    BeadList& getBeadList() { return beadChain.lst; }
    BarzelBead& appendBlankBead() { return beadChain.appendBlankBead(); }
    bool isListEmpty() const { return beadChain.lst.empty(); }
    bool hasBeads() const { return !beadChain.lst.empty(); }
    BarzelBead& getLastBead() { return beadChain.lst.back(); }
    const BarzelBead& getLastBead() const { return beadChain.lst.back(); }

    const       std::string& getOrigQuestion() const { return questionOrig; }
    const ay::StrUTF8& getQuestionOrigUTF8() const { return questionOrigUTF8; } /// our utf8 representation of questionOrig

    uint64_t    getQueryId() const { return d_origQuestionId; }
    void        setQueryId( uint64_t i ) { d_origQuestionId = i ; }
    bool        isQueryIdValid() const { return std::numeric_limits<uint64_t>::max() != d_origQuestionId; }
};
}
#endif // BARZER_BARZ_h
