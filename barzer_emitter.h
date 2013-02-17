/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_el_parser.h>
#include <barzer_el_btnd.h>


#include <ay/ay_logger.h>
namespace barzer{

typedef BELVarInfo::value_type VarVec;

struct PatternEmitterNode {
    virtual bool step() = 0;
    virtual void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const = 0;
    virtual ~PatternEmitterNode() {}

    static PatternEmitterNode* make(const BELParseTreeNode& node, VarVec &vars);
};

struct BELParseTreeNode_PatternEmitter {
    BTND_PatternDataVec curVec;
    BELVarInfo varVec;
    
    void reserveVecs( size_t sz = 8 ) 
    {
        curVec.reserve(sz);
        varVec.reserve(sz);
    }
    const BELParseTreeNode& tree;
    
    BELParseTreeNode_PatternEmitter( const BELParseTreeNode& t ) : tree(t)
        { makePatternTree(); }



    // the next 3 functions should only be ever called in the order they
    // are declared. It's very important

    const BTND_PatternDataVec& getCurSequence( )
    {
        if( patternTree )
            patternTree->yield(curVec, varVec);
        return curVec;
    }

    // should only be called after getCurSequence and before produceSequence
    const BELVarInfo& getVarInfo() const { return varVec; }

    /// returns false when fails to produce a sequence
    bool produceSequence();

    
    ~BELParseTreeNode_PatternEmitter();
private:
    PatternEmitterNode* patternTree;
    void makePatternTree();
};

std::ostream& btnd_xml_print( std::ostream&, const BELTrie&, const BTND_PatternData& d );

template<class T>class BELExpandReader : public BELReader {
    T &functor;
public:
    BELExpandReader(T &f, BELTrie* t, GlobalPools &gp, std::ostream* errStream )
        : BELReader(t,gp,errStream), functor(f) {}
    void addStatement( const BELStatementParsed& sp)
    {
        BELParseTreeNode_PatternEmitter emitter( sp.pattern );
        do {
            const BTND_PatternDataVec& seq = emitter.getCurSequence();
            functor(seq);
        } while( emitter.produceSequence() );
        ++numStatements;
    }
};

  
/// this emits combinations rather than adding 
class BELReaderXMLEmit : public BELReader {
    std::ostream& d_outStream;
public:
    BELReaderXMLEmit( BELTrie* t, std::ostream& os );
    void addStatement( const BELStatementParsed& sp );

};

/// this counts possible emits
class BELReaderXMLEmitCounter: public BELReader {
    std::ostream& d_outStream;
public:
    enum {MAX_CONSIDERED = 1024 };        
private: 
    size_t d_maxConsidered, d_counter;
public:
    BELReaderXMLEmitCounter(BELTrie* t, std::ostream& os ) ;

    size_t  maxConsidered() const { return d_maxConsidered; }
    void    setMaxConsidered( size_t x ) { d_maxConsidered= x; }
    

    void resetCounter() { d_counter= 0; }
    size_t getCounter() const { return d_counter; }

    void addStatement(const barzer::BELStatementParsed& sp);
    size_t power(const BELParseTreeNode& node) const;
    enum {INF = 0xffffffff};
    
};

class BELStatementParsed_EmitCounter {
    const BELTrie* d_trie; // this CAN be 0 . make sure to 0 check 
    size_t d_maxConsidered;
public:
    size_t power(const BELParseTreeNode& node) const;
    BELStatementParsed_EmitCounter( const BELTrie* trie, size_t x = BELReaderXMLEmitCounter::MAX_CONSIDERED ) : 
        d_trie(trie) , d_maxConsidered(x)
    {}
    size_t  maxConsidered() const { return d_maxConsidered; }
};

} // namespace barzer
