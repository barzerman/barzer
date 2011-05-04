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
#include <ay/ay_util.h>


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
	// const BarzelWildcardPool& d_wcPool;
	const BELTrie& d_trie;
	
	BarzelFCMap::const_iterator d_firmIter;
	const BarzelWCLookup* 	    d_wcLookup;
	const BarzelFCMap* 		    d_firmMap;

	BarzelWCLookup::const_iterator d_wcLookupIter;
	
	const BarzelFCMap*   getFirmMap() {
		return d_firmMap;
	}
public:
	typedef std::pair< const BarzelWCLookup*, BarzelWCLookup::const_iterator > LookupKey;
	typedef boost::variant< 
		BarzelFCMap::const_iterator,
		LookupKey
	> NodeKey;
	bool isOnFirmChildren() const { return ( d_firmMap && d_firmIter != d_firmMap->end() ); }	

	const BarzelTrieNode* getCurrentChild( NodeKey& k ) const
	{
		if( isOnFirmChildren() ) {
			k = d_firmIter;
			return ( &( d_firmIter->second ) );
		} else if( d_wcLookup && d_wcLookupIter != d_wcLookup->end() ) {
			k = LookupKey( d_wcLookup, d_wcLookupIter );
			return ( &( d_wcLookupIter->second ));
		} else {
			k = LookupKey( 0, BarzelWCLookup::const_iterator() );
			return ( 0 );
		}
	}
	const BarzelTrieNode* getCurrentChild( ) const
	{
		NodeKey k;
		return getCurrentChild( k );
	}
	const BarzelTrieNode* getNextChild(NodeKey& k) 
	{
		if( d_firmMap && d_firmIter != d_firmMap->end() && ++d_firmIter != d_firmMap->end() ) {
			k = d_firmIter;
			return ( &(d_firmIter->second) );
		} else if( d_wcLookup && d_wcLookupIter != d_wcLookup->end()  && ++d_wcLookupIter != d_wcLookup->end() ) {
			k = LookupKey( d_wcLookup, d_wcLookupIter );
			return ( &(d_wcLookupIter->second) );
		}

		k = LookupKey( 0, BarzelWCLookup::const_iterator() );
		return 0;
	}
	const BarzelTrieNode* getNextChild( )
	{
		NodeKey k;
		return getNextChild( k );
	}
	
	// bring everything current 
	void reset()
	{ 
		if( d_firmMap )
			d_firmIter = d_firmMap->begin(); 
		if( d_wcLookup ) 
			d_wcLookupIter = d_wcLookup->begin();
	}
	BarzelTrieNodeChildIterator( 
		const BarzelTrieNode& tn,
		const BELTrie& trie
		) : 
		d_tn(tn) ,
		d_trie(trie),
		// d_wcPool(wcPool),
		d_wcLookup(trie.getWCPool().getWCLookup( d_tn.getWCLookupId() ) ),
		d_firmMap( trie.getBarzelFCMap( d_tn ) )
	{ reset(); }

	BarzelTrieNodeChildIterator( const BarzelTrieNodeChildIterator& ni, const BarzelTrieNode& tn ) :
		d_tn(tn),
		d_trie(ni.d_trie),
		// d_wcPool(ni.d_wcPool),
		d_wcLookup(ni.d_trie.getWCPool().getWCLookup( d_tn.getWCLookupId() ) )
	{ reset(); }
};


/// BarzelTrie depth first traverser
/// does depth first traversal. 
/// presumes bool T::operator() ( const BarzelTrieNode& ) {} 
/// traversal stops as soon as false is returned and the d_stopNode is set to the node at which the stoppage 
/// occured is if traverser went over the whole trie - 0 is returned
struct BarzelTrieTraverser_depth {
	// const BarzelWildcardPool& d_wcPool;
	BELTrie d_trie;

	BarzelTrieTraverser_depth( const BarzelTrieNode& tn, const BELTrie& trie  )  :
		//d_wcPool(wcPool)
		d_trie(trie)
	{}

	typedef BarzelTrieNodeChildIterator::NodeKey   NodeKey;
	typedef std::vector< NodeKey >  NodeKeyVec;
	NodeKeyVec d_stack;

	const NodeKeyVec& getPath() { return d_stack; }
	/// visits every node in the tree until cb returns false, keeps the stack 
	template <typename T>
	const BarzelTrieNode* traverse(T& cb, const BarzelTrieNode& tn )
	{
		BarzelTrieNodeChildIterator it( tn, d_trie );
		NodeKey nk;
		for( const BarzelTrieNode* child = it.getCurrentChild(nk); child; child = it.getNextChild(nk) ) {
			ay::vector_raii< NodeKeyVec > raii( d_stack, nk );
			if( !cb(*child) )
				return child;
			const BarzelTrieNode* stopNode = traverse( cb, *child);
			if( stopNode ) 
				return stopNode;
		}
		return 0;
	}
	/// same as traverse except the stack is not kept
	// !!DO NOT USE THIS!! 
	template <typename T>
	const BarzelTrieNode* traverse_nostack(T& cb, const BarzelTrieNode& tn )
	{
		BarzelTrieNodeChildIterator it( tn, d_trie );
		for( const BarzelTrieNode* child = it.getCurrentChild(); child; child = it.getNextChild() ) {
			if( !cb(*child) )
				return child;
			const BarzelTrieNode* stopNode = traverse_nostack( cb, *child);
			if( stopNode ) 
				return stopNode;
		}
		return 0;
	}
};

struct BarzelTrieStatsCounter {
	size_t numNodes, numWildcards, numLeaves;

	bool operator()( const BarzelTrieNode& tn );
	BarzelTrieStatsCounter() :
		numNodes(0), numWildcards(0), numLeaves(0)
	{}
	std::ostream& print( std::ostream& fp ) const
	{
		return fp << 
		"Total nodes: " << numNodes << "\n" <<
		"Total wildcards: " << numWildcards << "\n" <<
		"Total leaves: " << numLeaves ;
		
	}
};
inline std::ostream& operator<<( std::ostream& fp, const BarzelTrieStatsCounter& c )
{ return c.print(fp); }

}
#endif /* BEL_TRIE_WALKER_H_ */
