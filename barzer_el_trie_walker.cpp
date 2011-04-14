/*
 * barzer_el_trie_walker.cpp
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#include "barzer_el_trie_walker.h"

namespace barzer {

TrieNodeStack& BELTrieWalker::getNodeStack() { return nodeStack; }

const BarzelWCLookup* BELTrieWalker::getWildcardLookup( uint32_t id ) const
{
	return( trie.wcPool->getWCLookup( id ));
}


bool BELTrieWalker::moveBack()
{
	if ( nodeStack.size() > 1 ) {
		nodeStack.pop();
		loadFC();
		loadWC();
		return true;
	} else return false;
}

int BELTrieWalker::moveTo(const size_t pos) 
{
	if( pos < fcvec.size()) {
		BarzelFCMap &fcmap = nodeStack.top()->getFirmMap();
		nodeStack.push(&fcmap[fcvec[pos]]);
		loadFC();
		loadWC();
		return 0;
	} else
		return 1;
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
		AYLOG(DEBUG) << i << " firm children loaded";
	}
}

void BELTrieWalker::loadWC() {
	wcvec.clear();
	const BarzelWCLookup *wcmap = getWildcardLookup(getCurrentNode().getWCLookupId());
	AYLOGDEBUG(wcmap->size());
	int i = 0;
	if (!wcmap->empty()) {
		for (BarzelWCLookup::const_iterator wci = wcmap->begin(); wci != wcmap->end(); ++wci) {
			wcvec.push_back(wci->first);
			i++;
		}
		AYLOG(DEBUG) << i << " wildcard children loaded";
	}
}

}

