#include <barzer_emitter.h>
#include <barzer_universe.h>
#include <ay/ay_debug.h>
namespace barzer {
//// 
void BELReaderXMLEmit::addStatement(const barzer::BELStatementParsed& sp)
{
    BELParseTreeNode_PatternEmitter emitter( sp.pattern );
    int i =0;
    d_outStream << "<rule n=\"" <<  sp.getStmtNumber() <<"\">\n";
    do {
        const BTND_PatternDataVec& seq = emitter.getCurSequence();
        if( !seq.size() ) 
            continue;
        d_outStream << "    <pat n=\"" << i << "\">";
        for( BTND_PatternDataVec::const_iterator pi = seq.begin(); pi != seq.end();++pi ) 
            btnd_xml_print( d_outStream , getTrie(), *pi );
        d_outStream << "</pat>\n";
        i++;
        //AYLOG(DEBUG) << "path added";
    } while( emitter.produceSequence() );
    d_outStream << "</rule>\n";
}


BELReaderXMLEmit::BELReaderXMLEmit( BELTrie* t, std::ostream& os ) :
        BELReader( t, t->getGlobalPools(), &os ),
        d_outStream(os)
    {
    }


// BELReaderXMLEmitCounter implementation

BELReaderXMLEmitCounter::BELReaderXMLEmitCounter( BELTrie* t, std::ostream& os ) :
        BELReader( t, t->getGlobalPools(), &os ),
        d_outStream(os)
{
}

// split 
void BELReaderXMLEmitCounter::addStatement(const barzer::BELStatementParsed& sp)
{
    BELParseTreeNode node = sp.pattern;
    d_outStream <<"\n" << power(node) <<std::endl;
}


namespace {
struct SumProductFunctor
{
  bool isSum;
  size_t& x;
  const BELReaderXMLEmitCounter& c;
  SumProductFunctor(const BELReaderXMLEmitCounter& counter, size_t& xx, bool i=true  ) 
  :isSum(i),x(xx), c(counter)  
  {x = isSum? 0:1;}
  size_t operator() (const barzer::BELParseTreeNode& node ) const
  {
    if (x > BELReaderXMLEmitCounter::MAX_CONSIDERED) return x=BELReaderXMLEmitCounter::INF;
    size_t p = c.power(node);
    if (p > BELReaderXMLEmitCounter::MAX_CONSIDERED) return x=BELReaderXMLEmitCounter::INF;
    else return x=(isSum? std::plus<size_t>()(x,p) : (p==0? std::multiplies<size_t>()(x,1):std::multiplies<size_t>()(x,p)));
  }
};


size_t factorial( size_t n )
{
  const size_t answer[] = {1,1,2,6,24,120,720,5040, 40320, 362880, 3628800, 39916800, 479001600};
  if (n > 12) return BELReaderXMLEmitCounter::INF;
  else return (answer[n] > BELReaderXMLEmitCounter::MAX_CONSIDERED) ? BELReaderXMLEmitCounter::INF : answer[n];
}

} // anon namespace 
/// return BELReaderXMLEmitCounter::INF if the result exceedes BELReaderXMLEmitCounter::MAX_CONSIDERED
size_t BELReaderXMLEmitCounter::power(const barzer::BELParseTreeNode& node) const
{


    switch (node.btndVar.which()) {
    case BTND_StructData_TYPE: {
      if (node.child.empty()) return 0;
      const BTND_StructData &sdata = boost::get<BTND_StructData>(node.btndVar);
      size_t p = 0;
      size_t x = 0;
      SumProductFunctor aggr(*this,p,(sdata.getType() == BTND_StructData::T_ANY) );
      std::for_each( node.child.begin(), node.child.end(), aggr);
      
        switch (sdata.getType()) {
        case BTND_StructData::T_LIST:   x = 1;                              break;
        case BTND_StructData::T_ANY:    x = 1;                              break;
        case BTND_StructData::T_OPT:    x = 2;                              break;  
        case BTND_StructData::T_PERM:   x = factorial(node.child.size());   break;
        case BTND_StructData::T_TAIL:   x = node.child.size();              break;
        case BTND_StructData::T_SUBSET: x = exp2(node.child.size());        break;
        default:
            AYLOG(ERROR) << "Invalid BTND_StructData type: " << sdata.getType();
            return 0;
        }
        //debug: std::cout << "node="<< sdata.getType() <<" p=" <<p<<" x="<<x<< std::endl;
        size_t answer = p*x;
        return (p>=MAX_CONSIDERED || x >=MAX_CONSIDERED || answer >=MAX_CONSIDERED)?  INF : answer;
    }
    case BTND_PatternData_TYPE:
        return 1;
    }
    AYLOG(ERROR) << "Smth definitly broken.Invalid BTND_TYPE type: " << node.btndVar.which();
    return 0;
}

}