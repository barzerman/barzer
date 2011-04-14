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

namespace barzer {

typedef std::stack<BarzelTrieNode*> TrieNodeStack;

class BELTrieWalker {
	BELTrie &trie;
	TrieNodeStack nodeStack;

public:
	BELTrieWalker(BELTrie &t) : trie(t) {
		nodeStack.push(&t.root);
	}

	TrieNodeStack& getNodeStack();
	bool moveBack();

	BarzelTrieNode* currectNode() {
		return nodeStack.top();
	}



};

}
#endif /* BEL_TRIE_WALKER_H_ */
