/*
 * barzer_el_trie_walker.cpp
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#include "barzer_el_trie_walker.h"

namespace barzer {

TrieNodeStack& BELTrieWalker::getNodeStack() { return nodeStack; }

bool BELTrieWalker::moveBack()
{
	if ( nodeStack.size() > 1 ) {
		nodeStack.pop();
		loadFC();
		return true;
	} else return false;
}

int BELTrieWalker::moveTo(const size_t pos) {
	if (pos < 0 || pos >= fcvec.size()) return 1;

	BarzelFCMap &fcmap = nodeStack.top()->getFirmMap();
	nodeStack.push(&fcmap[fcvec[pos]]);
	loadFC();
	return 0;
}

void BELTrieWalker::loadFC() {
	fcvec.clear();
	BarzelFCMap &fcmap = nodeStack.top()->getFirmMap();
	AYLOGDEBUG(fcmap.size());
	int i = 0;
	if (!fcmap.empty()) {
		for (BarzelFCMap::iterator fci = fcmap.begin(); fci != fcmap.end(); ++fci) {
			fcvec.push_back(fci->first);
			i++;
		}
		AYLOG(DEBUG) << i << " children loaded";
	}



}

}

