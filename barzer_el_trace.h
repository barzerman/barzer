#pragma once

namespace barzer {
struct BarzelTrace {
    enum { 
        MAX_TAIL_REPEAT = 64  // if more than this many consecutive occurrences are detected its a loop
    }; 
    struct SingleFrameTrace {
        size_t grammarSeqNo; // grammar sequence number
        BarzelTranslationTraceInfo tranInfo; // translation trace info for the rule that was hit
        uint32_t globalTriePoolId; /// global trie pool id of the trie that was hit 
        uint32_t tranId;  // translation id

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
        SingleFrameTrace( ) : grammarSeqNo(0),globalTriePoolId(0xffffffff), tranId(0xffffffff) {}
        SingleFrameTrace( const BarzelTranslationTraceInfo& ti, size_t n, uint32_t trieId, uint32_t trId ) : grammarSeqNo(n), tranInfo(ti),globalTriePoolId(trieId), tranId(trId) {}
    };

    typedef std::vector< SingleFrameTrace > TraceVec;
    
    size_t d_grammarSeqNo; // current grammar sequence number 

    SingleFrameTrace d_lastFrame;
    size_t d_lastFrame_count;

    enum : int {
        ERRBIT_TRUNCATED_LENGTH, /// 
        ERRBIT_TRUNCATED_TOKENS, /// 
        /// add new error bits above this
        ERRBIT_MAX
    };
    ay::bitflags< ERRBIT_MAX > d_errBit;

    TraceVec d_tvec;
    std::string skippedTriesString;

    void clear() { 
        d_tvec.clear(); 
        skippedTriesString.clear();  
        d_lastFrame_count=0;
        d_errBit.clear();
    }
    void setErrBit( int b ) { if( b< ERRBIT_MAX ) d_errBit.set( b ); }
    bool checkErrBit( int b ) const { return d_errBit.checkBit( b ); }

    size_t size() const { return d_tvec.size(); }
    void push_back( const BarzelTranslationTraceInfo& traceInfo, uint32_t trieId, uint32_t tranId  ) 
    {
        d_tvec.push_back( SingleFrameTrace( traceInfo, d_grammarSeqNo, trieId, tranId ) );
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
    
    void setError( const char* err ) ;
    // max number of errors to report per trace
    enum { BARZEL_TRACE_MAX_ERR=8 };
    void pushError( const char* err ) ;
    bool detectLoop() const;
    bool lastFrameLoop() const { return (d_lastFrame_count>= MAX_TAIL_REPEAT); }
}; 
} // namespace barzer
