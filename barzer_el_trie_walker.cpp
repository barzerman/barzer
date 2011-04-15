/*
 * barzer_el_trie_walker.cpp
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#include "barzer_el_trie_walker.h"

namespace barzer {

TrieNodeStack& BELTrieWalker::getNodeStack() { return nodeStack; }

BarzelWCLookup* BELTrieWalker::getWildcardLookup( uint32_t id )
{
	return( trie.wcPool->getWCLookup( id ));
}


bool BELTrieWalker::moveBack()
{
	if ( nodeStack.size() > 1 ) {
		nodeStack.pop();
		loadChildren();
		return true;
	} else return false;
}

int BELTrieWalker::moveToFC(const size_t pos)
{
	if( pos < fcvec.size()) {
		//BarzelTrieNode &node = ;
		BarzelFCMap &fcmap = getCurrentNode().getFirmMap();
		//BarzelTrieNode &nextNode = ;
		nodeStack.push(&fcmap[fcvec[pos]]);
		loadChildren();
		return 0;
	} else
		return 1;
}


int BELTrieWalker::moveToWC(const size_t pos)
{
	if( pos < wcNodeVec.size()) {
		nodeStack.push(wcNodeVec[pos]);
		loadChildren();
		return 0;
	} else
		return 1;
}

void BELTrieWalker::loadChildren() {
	loadFC();
	loadWC();
}

void BELTrieWalker::loadFC() {
	fcvec.clear();
	BarzelTrieNode &node = getCurrentNode();
	BarzelFCMap &fcmap = node.getFirmMap();
	AYLOGDEBUG(fcmap.size());
 	int i = 0;
	if (node.hasFirmChildren()) {
		for (BarzelFCMap::const_iterator fci = fcmap.begin();
				fci != fcmap.end();	++fci) {
			fcvec.push_back(fci->first);
			i++;
		}
		AYLOG(DEBUG) << i << " firm children loaded";
	}
}

void BELTrieWalker::loadWC() {
	wcKeyVec.clear();
	wcNodeVec.clear();
	BarzelWCLookup *wcmap = getWildcardLookup(getCurrentNode().getWCLookupId());
	if (!wcmap) return;
	AYLOGDEBUG(wcmap->size());
	int i = 0;
	if (!wcmap->empty()) {
		for (BarzelWCLookup::iterator wci = wcmap->begin(); wci != wcmap->end(); ++wci) {
			wcKeyVec.push_back(wci->first);
			BarzelTrieNode &node = wci->second;
			wcNodeVec.push_back(&node);
			i++;
		}
		AYLOG(DEBUG) << i << " wildcard children loaded";
	}
}

}

