/*
 * bel_trie_walker.h
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#ifndef BEL_TRIE_WALKER_H_
#define BEL_TRIE_WALKER_H_

#include <barzer_el_trie.h>
#include <barzer_el_wildcard.h>
#include <stack>
#include <vector>
#include <ay/ay_logger.h>


namespace barzer {

//typedef std::stack<BarzelTrieNode*> TrieNodeStack;

///*

typedef boost::variant<
    BarzelTrieFirmChildKey,
    BarzelWCLookupKey
> BELTrieWalkerKey;

typedef std::pair<BELTrieWalkerKey,const BarzelTrieNode*> BELTWStackPair;
typedef std::vector<BELTWStackPair> TrieNodeStack;
//*/

class BELTrieWalker {
	const BELTrie &trie;
	TrieNodeStack nodeStack;
	std::vector<BarzelTrieFirmChildKey> fcvec;

	std::vector<BarzelWCLookupKey> wcKeyVec;
	std::vector<BarzelTrieNode*> wcNodeVec;

public:
	BELTrieWalker(BELTrie &t) : trie(t) {
		nodeStack.push_back(BELTWStackPair(BELTrieWalkerKey(BarzelTrieFirmChildKey()), &t.root));
		//nodeStack.push(&t.root);
	}

	TrieNodeStack& getNodeStack();
	const BarzelTrieNode& getCurrentNode() { return *(nodeStack.back().second); }
	//BarzelTrieNode& getCurrentNode() { return *(nodeStack.top()); }


	BarzelWCLookup* getWildcardLookup(uint32_t);


	void loadChildren();
	void loadFC();
	void loadWC();
	bool moveBack();
	int moveToFC(size_t);
	int moveToWC(size_t);

	std::vector<BarzelTrieFirmChildKey>& getFCvec() { return fcvec; }
	std::vector<BarzelWCLookupKey>& getWCvec() { return wcKeyVec; }
};

class BarzelTrieNodeChildIterator {
	const BarzelTrieNode& d_tn;
	const BarzelWildcardPool& d_wcPool;
	
	BarzelFCMap::const_iterator d_firmIter;
	const BarzelWCLookup* 	    d_wcLookup;
	BarzelWCLookup::const_iterator d_wcLookupIter;
public:
	bool isOnFirmChildren() const { return d_firmIter != d_tn.getFirmMap().end() ; }	

	const BarzelTrieNode* getCurrentChild( ) const
	{
		if( isOnFirmChildren() ) {
			return ( &( d_firmIter->second ) );
		} else if( d_wcLookup && d_wcLookupIter != d_wcLookup->end() ) {
			return ( &( d_wcLookupIter->second ));
		} else
			return ( 0);
	}
	const BarzelTrieNode* getNextChild( )
	{
		if( isOnFirmChildren() ) {
			if( ++d_firmIter != d_tn.getFirmMap().end() )
				return ( &(d_firmIter->second) );
			else if( d_wcLookup && d_wcLookupIter != d_wcLookup->end() ) {
				if( ++d_wcLookupIter != d_wcLookup->end()  )
					return ( &(d_wcLookupIter->second) );
			}
		}
		return 0;
	}
	// bring everything current 
	void reset()
	{ 
		d_firmIter = d_tn.getFirmMap().begin(); 
		if( d_wcLookup ) 
			d_wcLookupIter = d_wcLookup->begin();
	}
	BarzelTrieNodeChildIterator( 
		const BarzelTrieNode& tn,
		const BarzelWildcardPool& wcPool
		) : 
		d_tn(tn) ,
		d_wcPool(wcPool),
		d_wcLookup(d_wcPool.getWCLookup( d_tn.getWCLookupId() ) )
	{ reset(); }

	BarzelTrieNodeChildIterator( const BarzelTrieNodeChildIterator& ni, const BarzelTrieNode& tn ) :
		d_tn(tn),
		d_wcPool(ni.d_wcPool),
		d_wcLookup(d_wcPool.getWCLookup( d_tn.getWCLookupId() ) )
	{ reset(); }
};

/// BarzelTrie depth first traverser
/// does depth first traversal. 
/// presumes bool T::operator() ( const BarzelTrieNode& ) {} 
/// traversal stops as soon as false is returned and the d_stopNode is set to the node at which the stoppage 
/// occured is if traverser went over the whole trie - 0 is returned
struct BarzelTrieTraverser_depth {
	const BarzelWildcardPool& d_wcPool;

	BarzelTrieTraverser_depth( const BarzelTrieNode& tn, const BarzelWildcardPool& wcPool  )  :
		d_wcPool(wcPool)
	{}

	template <typename T>
	const BarzelTrieNode* traverse(T& cb, const BarzelTrieNode& tn )
	{
		BarzelTrieNodeChildIterator it( tn, d_wcPool );
		for( const BarzelTrieNode* child = it.getCurrentChild(); child; child = it.getNextChild() ) {
			if( !cb(*child) )
				return child;
			const BarzelTrieNode* stopNode = traverse( cb, *child);
			if( stopNode ) 
				return stopNode;
		}
		return 0;
	}
};

struct BarzelTrieStatsCounter {
	size_t numNodes, numWildcards, numLeaves;

	bool operator()( const BarzelTrieNode& tn )
	{
		++numNodes;
		if( tn.isLeaf() ) {
			++numLeaves;
		}
		if( tn.isWcChild() ) {
			++numWildcards;
		}
		return true;
	}
	BarzelTrieStatsCounter() :
		numNodes(0), numWildcards(0), numLeaves(0)
	{}
	std::ostream& print( std::ostream& fp ) const
	{
		return fp << 
		"Total nodes: " << numNodes << "\n" <<
		"Total wildcards: " << numWildcards << "\n" <<
		"Total leaves: " << numWildcards ;
		
	}
};
inline std::ostream& operator<<( std::ostream& fp, const BarzelTrieStatsCounter& c )
{ return c.print(fp); }

}
#endif /* BEL_TRIE_WALKER_H_ */
