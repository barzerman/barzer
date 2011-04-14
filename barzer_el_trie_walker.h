/*
 * bel_trie_walker.h
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#ifndef BEL_TRIE_WALKER_H_
#define BEL_TRIE_WALKER_H_

#include "barzer_el_trie.h"
#include <stack>
#include <vector>
#include <ay/ay_logger.h>


namespace barzer {

typedef std::stack<BarzelTrieNode*> TrieNodeStack;

class BELTrieWalker {
	BELTrie &trie;
	TrieNodeStack nodeStack;
	std::vector<BarzelTrieFirmChildKey> fcvec;

public:
	BELTrieWalker(BELTrie &t) : trie(t) {
		nodeStack.push(&t.root);
	}

	TrieNodeStack& getNodeStack();
	BarzelTrieNode& getCurrentNode() {	return *nodeStack.top(); }



	void loadFC();
	bool moveBack();
	int moveTo(size_t);

	std::vector<BarzelTrieFirmChildKey>& getFCvec() { return fcvec; }

	BarzelTrieNode* currectNode() {
		return nodeStack.top();
	}



};

}
#endif /* BEL_TRIE_WALKER_H_ */
