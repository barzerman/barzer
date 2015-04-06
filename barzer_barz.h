#pragma once
#include <barzer_el_chain.h>
#include <barzer_parse_types.h>
#include <barzer_topics.h>
#include <barzer_tokenizer.h>
#include <barzer_el_trace.h>
#include <ay/ay_utf8.h>

namespace barzer {

class QSemanticParser;
class QLexParser;
class QTokenizer;
class StoredUniverse;
struct MatcherCallback;

struct RequestVariableMap;
struct RequestEnvironment; // server request environment

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
    const StoredUniverse* getUniverse() const { return d_universe; }
};

/// confidence data and alternative representations
/// assuming three levels of confidence LOW,MEDIUM,HIGH
/// contains the data 
struct  BarzConfidenceData {
   typedef std::vector< std::pair<size_t,size_t> >  OffsetAndLengthPairVec;
   
   size_t d_hiCnt, d_medCnt, d_loCnt; 

   bool hasAnyConfidence() const { return ( d_hiCnt|| d_medCnt|| d_loCnt ); } 

   OffsetAndLengthPairVec d_noHi  // no highs
                        , d_noMed // no mediums or lows 
                        , d_noLo; // no lows , meds or highs

    void clear() 
    { 
        d_hiCnt= d_medCnt= d_loCnt=0;
        d_noHi.clear();
        d_noMed.clear();
        d_noLo.clear();
    }
    
    BarzConfidenceData() : d_hiCnt(0) , d_medCnt(0), d_loCnt(0) {}
    void sortAndInvert( const std::string& origStr, OffsetAndLengthPairVec&  );
    void sortAndInvert( const std::string& origStr );
    void fillString( std::vector<std::string>& dest, const std::string& src, int conf ) const;
};

struct BENIResult {
    BENIFindResults_t d_entVec;

    BENIFindResults_t d_zurchEntVec;

    bool empty()            const { return d_entVec.empty(); }
    bool zurchResultEmpty() const { return d_zurchEntVec.empty(); }
    void clear() { 
        d_entVec.clear(); 
        d_zurchEntVec.clear();
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
    const char* getReqVarAsChars( const char* ) const;
public:
    BENIResult d_beni;
    const BENIFindResults_t& getBeniResults() const { return d_beni.d_entVec; }
    BarzConfidenceData confidenceData;

    /// given offset and length in bytes in the original quesstion 
    /// returns a pair(glyph, lengthinGlyphs) - for utf8
    std::pair<size_t,size_t> getGlyphFromOffsets( size_t offset, size_t length ) const;

	enum { 
        MAX_TRACE_LEN = 256, 
        LONG_TRACE_LEN = 32  
    };

	BarzelTrace barzelTrace;
    BarzTopics  topicInfo;
    const StoredUniverse* getUniverse() const { return m_hints.getUniverse(); } 
    Barz();

    void setServerReqEnv( RequestEnvironment* env ) { d_serverReqEnv = env; }
    RequestEnvironment* getServerReqEnv() { return d_serverReqEnv; }
    const BarzerDateTime*  getNowPtr() const;
    const RequestEnvironment* getServerReqEnv() const { return d_serverReqEnv; }

    /// working with request variables - these variables reside in the top level request context. they live through the whole request
    /// and aren't tied to any one rewrite
    const RequestVariableMap* getRequestVariableMap()  const ;
    RequestVariableMap* getRequestVariableMap() ;

    bool getReqVarValue( BarzelBeadAtomic_var& v, const char* n ) const;
    bool getReqVarValue( BarzerString& str, const char* n ) const;
    bool hasReqVar( const char* ) const;
    /// true if request variable n exists and is equal to val
    bool hasReqVarEqualTo( const char* n, const char* val ) const;
    /// true if request variable n exists and is NOT equal to val 
    bool hasReqVarNotEqualTo( const char* n, const char* val ) const;

    const BarzelBeadAtomic_var*  getReqVarValue( const char* n ) const;
    void setReqVarValue( const char* n, const BarzelBeadAtomic_var& v );
    bool unsetReqVar( const char* n );
    //// end of request variable context functions 

	void setUniverse(const StoredUniverse*);
	void assignQuery(const char*);
	const BarzHints& getHints() const;

    bool lastFrameLoop() const { return barzelTrace.lastFrameLoop(); }
    void pushError( const char* err ) { barzelTrace.pushError(err); }
    void setError( const char* err ) { barzelTrace.setError(err); }
    
    bool hasProperties() const { return topicInfo.hasProperties(); }
    bool hasZeroUniverseProperties() const { return topicInfo.hasZeroUniverseProperties(); }

    // analyzing trace to determine whether something is wrong and we're in a rewriting loop
    bool shouldTerminate() const;

	void pushTrace( const BarzelTranslationTraceInfo& trace, uint32_t trieId, uint32_t tranId );

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

    struct ConfidenceMode {
        enum {
            MODE_CONFIDENCE, 
            MODE_ENTITY
        };
        int mode; 

        StoredEntityClass eclass;
        ConfidenceMode() : mode(MODE_CONFIDENCE) {}
        ConfidenceMode( const StoredEntityClass& ec ) : mode(MODE_ENTITY), eclass(ec) {}
    };
    int computeConfidence( const StoredUniverse& u, const QuestionParm& qparm, const char* q, const ConfidenceMode& mode =ConfidenceMode());
    int beniSearch( const StoredUniverse& u, const QuestionParm& qparm );

	/// returns pair. first is the number of units which have been modified semantically
	/// mening - fluff, date, entity, erc, expression etc
	// second - number of beads in the barz 
	std::pair<size_t,size_t> getBeadCount() const;
    const BeadList& getBeadList() const { return beadChain.lst; }
    BeadList& getBeadList() { return beadChain.lst; }
    BarzelBead& appendBlankBead() { return beadChain.appendBlankBead(); }
    bool isListEmpty() const { return beadChain.lst.empty(); }
    bool hasBeads() const { return !beadChain.lst.empty(); }
    BarzelBead& getLastBead() { return beadChain.lst.back(); }
    const BarzelBead& getLastBead() const { return beadChain.lst.back(); }


    const       std::string& getOrigQuestion() const { return questionOrig; }
    std::string getPositionalQuestion() const;
    const ay::StrUTF8& getQuestionOrigUTF8() const { return questionOrigUTF8; } /// our utf8 representation of questionOrig

    uint64_t    getQueryId() const { return d_origQuestionId; }
    void        setQueryId( uint64_t i ) { d_origQuestionId = i ; }
    bool        isQueryIdValid() const { return std::numeric_limits<uint64_t>::max() != d_origQuestionId; }
    
    int         extraNormalization( const QuestionParm& qparm );

    /// returns contiguous areas as separate strings
    void getSourceStrings( std::vector<std::string>& vec, std::vector<std::pair<size_t, size_t>>& ) const;

    /// returns an array of continuous offsets constructed from all offsets in all ttokens 
    void getSrctok( std::vector<std::string>&, const std::vector<std::pair<size_t,size_t>>& ) const;
    void getAllButSrctok( std::vector<std::string>& vec,  const std::vector<std::pair<size_t, size_t>>& posVec ) const;
    std::string getBeadSrcTok( const BarzelBead& bead ) const;
    void getContinuousOrigOffsets( const BarzelBead& bead, std::vector< std::pair<size_t, size_t> >& vec ) const;
    
    /// default maximum number of expansions for chain2string
    enum { CHAIN2STRING_MAX = 10 };
    /// returns the chain where numbers are reported as source tokens
    /// tokens are as is 
    /// entities from classes designated as beni synonyms - entity strings 
    /// everything else - skipped
    std::vector<std::string> chain2string( size_t combMax = CHAIN2STRING_MAX ) const;
};

} // namespace barzer
