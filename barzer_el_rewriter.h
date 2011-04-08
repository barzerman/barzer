#ifndef BARZER_EL_REWRITER_H
#define BARZER_EL_REWRITER_H
#include <barzer_el_btnd.h>
#include <barzer_el_parser.h>
namespace barzer {

/// rewriter is a tree which is evaluated based on input
/// result of evaluation is a value computed from underlying nodes
/// value is a constant and can be of one of the constant types 
/// string, compounded word, number, date, time, entity
/// range - range has range type and the actual to-from range 
///  - range type is either a hardcoded thing or an entity 
/// 
/// input is RewriterContext which has a hash table with variables and other 
/// stuff

/// tree encoding format
/// nodeopen 1byte 
//// nodeheader 4 bytes 
/// 
/// nodeclose 1byte
struct BarzelRwrTreeNode {
};
class BarzelTranslation;

/// unique at least per BarzelTrie (most likely globally unique)
/// the pool stores binary buffers with barzel rewrite instructions
/// these buffers are used to build BarzelRewriteTree objects on the fly 
class BarzelRewriterPool {
	typedef std::pair<char*, uint32_t> BufAndSize;
	typedef std::vector<uint8_t> byte_vec;
	typedef std::vector< BufAndSize > BufAndSizeVec;
	BufAndSizeVec encVec;

	uint32_t poolNewBuf( const uint8_t* s, uint32_t sz )
	{
		encVec.push_back( BufAndSize( (char*)malloc(sz), sz) );
		return (encVec.size() -1);
	}

	int  encodeParseTreeNode( BarzelRewriterPool::byte_vec& trans, const BELParseTreeNode& ptn );
public:
	~BarzelRewriterPool();
	BarzelRewriterPool( size_t reserveSz ) 
		{ encVec.reserve( reserveSz ) ; }


	enum {
		ERR_OK,
		ERR_ENCODING_FAILED, // input tree is invalid
		ERR_ENCODING_INCONSISTENT // some internal inconsistency encountered
	};
	// returns one of the ERR_XXX enums
	int produceTranslation( BarzelTranslation& , const BELParseTreeNode& ptn );
};

}

#endif // BARZER_EL_REWRITER_H
