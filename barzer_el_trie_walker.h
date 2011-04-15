/*
 * bel_trie_walker.h
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#ifndef BEL_TRIE_WALKER_H_
#define BEL_TRIE_WALKER_H_

#include "barzer_el_trie.h"
#include "barzer_el_wildcard.h"
#include <stack>
#include <vector>
#include <ay/ay_logger.h>


namespace barzer {

typedef std::stack<BarzelTrieNode*> TrieNodeStack;
typedef std::pair<BarzelWCLookupKey,BarzelTrieNode> WCRec;

class BELTrieWalker {
	BELTrie &trie;
	TrieNodeStack nodeStack;
	std::vector<BarzelTrieFirmChildKey> fcvec;

	std::vector<BarzelWCLookupKey> wcKeyVec;
	std::vector<BarzelTrieNode*> wcNodeVec;

public:
	BELTrieWalker(BELTrie &t) : trie(t) {
		nodeStack.push(&t.root);
	}

	TrieNodeStack& getNodeStack();
	BarzelTrieNode& getCurrentNode() { return *nodeStack.top(); }


	BarzelWCLookup* getWildcardLookup(uint32_t);


	void loadChildren();
	void loadFC();
	void loadWC();
	bool moveBack();
	int moveToFC(size_t);
	int moveToWC(size_t);

	std::vector<BarzelTrieFirmChildKey>& getFCvec() { return fcvec; }
	std::vector<BarzelWCLookupKey>& getWCvec() { return wcKeyVec; }

	BarzelTrieNode* currentNode() {
		return nodeStack.top();
	}

};

}
#endif /* BEL_TRIE_WALKER_H_ */
