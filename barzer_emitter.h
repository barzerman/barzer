#ifndef BARZER_EMITTER_H
#define BARZER_EMITTER_H

#include <barzer_el_parser.h>

#include <ay/ay_logger.h>
namespace barzer{
  
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
    BELReaderXMLEmitCounter(BELTrie* t, std::ostream& os );
    void addStatement(const barzer::BELStatementParsed& sp);
    size_t power(const BELParseTreeNode& node) const;
    enum {MAX_CONSIDERED = 100, INF = 0xffffffff};
};
  
}
#endif //BARZER_EMITTER_H