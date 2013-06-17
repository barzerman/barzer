
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#include <barzer_el_trie_ruleidx.h>
#include <barzer_el_trie.h>
#include <barzer_universe.h>
#include <ay/ay_parse.h>

namespace barzer
{
void TrieRuleIdx::addNode(const RulePath& path, BarzelTrieNode *node )
{
	auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		pos = m_path2nodes.insert({ path, TrieNodeInfo() }).first;

    for( const auto& i : pos->second ) {
        if( i == node )  // we dont add if it already exists 
            return;
    }

    pos->second.push_back( node );
}

bool TrieRuleIdx::removeNode( const char* trieClass, const char* trieId , const char* source, uint32_t statementId )
{
    if( BELTrie* trie = d_universe.getTrieCluster().getTrieByClassAndId( trieClass, trieId ) ) {
        uint32_t sourceId = d_universe.getGlobalPools().internalString_getId( source );

        RulePath path( statementId, sourceId, d_universe.getTrieCluster().getUniqueTrieId(trieClass,trieId) );
        return removeNode(path) ;
    } else
        return false;
    
    return true;
}

bool TrieRuleIdx::removeNode(const char* str)
{
    std::vector< std::string > arg;
    ay::separated_string_to_vec( arg, '|' )( str );
    if( arg.size() < 4 ) 
        return false;
    enum {
        XX_TRIE_CLASS,
        XX_TRIE_ID,
        XX_SOURCE,
        XX_STATEMENT
    };
    return removeNode( 
        arg[XX_TRIE_CLASS].c_str() ,
        arg[XX_TRIE_ID].c_str() ,
        arg[XX_SOURCE].c_str() ,
        atoi(arg[XX_STATEMENT].c_str())
    );
}

bool TrieRuleIdx::removeNode (const RulePath& path)
{
	const auto pos = m_path2nodes.find(path);
	if (pos == m_path2nodes.end())
		return false;

    if( BELTrie* trie = d_universe.getTrieCluster().getTrieByUniqueId(path.m_trieId) ) {
        AmbiguousTranslationReference& ambTranRef = trie->getAmbiguousTranslationReference();
        for( auto& trieNode : pos->second ) { // iterating over all nodes for this rule path
            size_t numAmbiguousTranslations = !ambTranRef.unlink( trieNode->getTranslationId(), path.source(), path.statementId() );
            
            if( numAmbiguousTranslations )
                continue;
            /// this translation is no longer ambiguous 
            trieNode->clearTranslation();
            /// this node doesnt have a translation anymore 
            /// now if it's childless we remove it and go up the trie 
            if( !trieNode->hasChildren() ) 
                trie->removeNode( trieNode );
        }

    } else 
        return false;


	for (auto& info : pos->second)
	{
		BarzelTrieNode* node = info;
	}

	m_path2nodes.erase(pos);
    return true;
}

} // namespace barzer 
