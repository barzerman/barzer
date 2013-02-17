
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_el_trie_walker.cpp
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#include <barzer_el_trie_walker.h>

namespace barzer {


BarzelWCLookup* BELTrieWalker::getWildcardLookup( uint32_t id )
{
	return( trie->getWCPoolPtr()->getWCLookup( id ));
}


bool BELTrieWalker::moveBack()
{
	if ( nodeStack.size() > 1 ) {
		nodeStack.pop_back();
		loadChildren();
		return true;
	} else return false;
}

void BELTrieWalker::setTrie( const BELTrie* t )
{
	if( !t )  {
		std::cerr << "trying to set NULL tree for the walker\n";
		return;
	}
	trie = t;
	nodeStack.clear();
	nodeStack.push_back(BELTWStackPair(BELTrieWalkerKey(BarzelTrieFirmChildKey()), &(trie->root) ));
	fcvec.clear();
	wcKeyVec.clear();
	wcNodeVec.clear();
	loadChildren();
}
int BELTrieWalker::moveToFC(const size_t pos)
{
	if( pos < fcvec.size()) {
		const BarzelFCMap *fcmap = trie->getBarzelFCMap( getCurrentNode() );

		if( fcmap ) {
			const BarzelTrieFirmChildKey &key = fcvec[pos];
	
			BarzelFCMap::const_iterator i = fcmap->find( key );
			if( i== fcmap->end() ) {
					AYTRACE("PANIC: invalid key");
				return 1;
			}
			nodeStack.push_back(BELTWStackPair(BELTrieWalkerKey(key), &(i->second) ));
		}
		//nodeStack.push(&fcmap[key]);
		loadChildren();
		return 0;
	} else
		return 1;
}


int BELTrieWalker::moveToWC(const size_t pos)
{
	if( pos < wcNodeVec.size()) {
		BELTrieWalkerKey key(wcKeyVec[pos]);
		nodeStack.push_back(BELTWStackPair(key, wcNodeVec[pos]));
		//nodeStack.push(wcNodeVec[pos]);
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
	const BarzelTrieNode &node = getCurrentNode();
	const BarzelFCMap *fcmap = trie->getBarzelFCMap( node );
	//AYLOGDEBUG(fcmap.size());
 	int i = 0;
	if (fcmap && node.hasFirmChildren()) {
		for (BarzelFCMap::const_iterator fci = fcmap->begin();
				fci != fcmap->end();	++fci) {
			fcvec.push_back(fci->first);
			i++;
		}
		//AYLOG(DEBUG) << i << " firm children loaded";
	}
}

void BELTrieWalker::loadWC() {
	wcKeyVec.clear();
	wcNodeVec.clear();
	BarzelWCLookup *wcmap = getWildcardLookup(getCurrentNode().getWCLookupId());
	if (!wcmap) return;
	//AYLOGDEBUG(wcmap->size());
	int i = 0;
	if (!wcmap->empty()) {
		for (BarzelWCLookup::iterator wci = wcmap->begin(); wci != wcmap->end(); ++wci) {
			wcKeyVec.push_back(wci->first);
			BarzelTrieNode &node = wci->second;
			wcNodeVec.push_back(&node);
			i++;
		}
		//AYLOG(DEBUG) << i << " wildcard children loaded";
	}
}

	bool BarzelTrieStatsCounter::operator()( const BarzelTrieNode& tn )
	{
		++numNodes;
		if( tn.isLeaf() ) 
			++numLeaves;
		if( tn.isWcChild() ) 
			++numWildcards;
		return true;
	}

}

