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

}
#endif /* BEL_TRIE_WALKER_H_ */
