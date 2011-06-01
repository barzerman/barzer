#include <barzer_el_analysis.h>
#include <barzer_el_trie.h>

namespace barzer {

void TrieAnalyzer::setNameThreshold( size_t n )
{
	d_nameThreshold = d_universe.getDtaIdx().getNumberOfEntities()/n;
}
void TrieAnalyzer::setFluffThreshold( size_t n ) 
{
	d_fluffThreshold= (1+ d_universe.getDtaIdx().getNumberOfEntities()/n);
}
TrieAnalyzer::TrieAnalyzer( const StoredUniverse& u ) :
	d_universe(u),
	d_trav( u.getBarzelTrie().getRoot(), u.getBarzelTrie() ),
	d_nameThreshold(1+u.getDtaIdx().getNumberOfEntities()/2000),
	d_fluffThreshold(1+u.getDtaIdx().getNumberOfEntities()/200)
{}

bool TrieAnalyzer::getPathTokens( std::vector< uint32_t >& tvec ) const 
{
	tvec.clear();
	const BarzelTrieTraverser_depth::NodeKeyVec& nkv = d_trav.getPath();
	for( BarzelTrieTraverser_depth::NodeKeyVec::const_iterator i = nkv.begin(); i != nkv.end(); ++i ) {
		const BarzelTrieTraverser_depth::NodeKey& nk = *i;
		/// nk is a variant - the 0 member is firm key , everything above it is a wildcard or other stuff
		/// if we encounter a non-firm key we will abort name creation 
		if( nk.which() ) {
			tvec.clear();
			return false;
		}
		/// here we're guaranteed that which is 0  
		const BarzelFCMap::const_iterator& fcmi = boost::get< BarzelFCMap::const_iterator > ( nk );
		const BarzelTrieFirmChildKey& fck = fcmi->first;
		uint32_t stringId = fck.getTokenStringId( ) ;
		if( stringId == 0xffffffff ) {
			tvec.clear();
			return false;
		}
		tvec.push_back( stringId );
	}
	return true;
}

bool TrieAnalyzer::getPathTokens( ay::char_cp_vec& tvec ) const 
{
	tvec.clear();
	const BarzelTrieTraverser_depth::NodeKeyVec& nkv = d_trav.getPath();
	for( BarzelTrieTraverser_depth::NodeKeyVec::const_iterator i = nkv.begin(); i != nkv.end(); ++i ) {
		const BarzelTrieTraverser_depth::NodeKey& nk = *i;
		/// nk is a variant - the 0 member is firm key , everything above it is a wildcard or other stuff
		/// if we encounter a non-firm key we will abort name creation 
		if( nk.which() ) {
			tvec.clear();
			return false;
		}
		/// here we're guaranteed that which is 0  
		const BarzelFCMap::const_iterator& fcmi = boost::get< BarzelFCMap::const_iterator > ( nk );
		const BarzelTrieFirmChildKey& fck = fcmi->first;
		uint32_t stringId = fck.getTokenStringId( ) ;
		if( stringId == 0xffffffff ) {
			tvec.clear();
			return false;
		}

		const char* tok = d_universe.decodeStringById( stringId );
		if( tok && *tok ) {
			tvec.push_back( tok );
		}
	}
	return true;
}

void TrieAnalyzer::updateAnalytics( BTN_cp tn, TA_BTN_data& dta )
{
	if( tn->isLeaf() ) {
		const BELTrie& trie = d_universe.getBarzelTrie();
		const BarzelTranslation* tran = trie.getBarzelTranslation( *tn );
		if( tran ) {
			if( tran->isMkEntSingle() ) {
				dta.addEntity( tran->getId_uint32() );
			} else if( tran->isMkEntList() ) {
				const EntityGroup* entGrp = trie.getEntityCollection().getEntGroup( tran->getId_uint32() );
				if( entGrp ) 
				dta.entities.insert( entGrp->getVec().begin(), entGrp->getVec().end());
			}
		}
	}
}

std::ostream& TrieAnalyzer::print( std::ostream& fp ) const
{
	for(BTNDataHash::const_iterator i = d_dtaHash.begin(); i!= d_dtaHash.end(); ++i ) {
		fp << "[" << i->first << "]" << ":" ;
		i->second.print( fp, d_universe ) ;
	}
	return fp;
}
bool TrieAnalyzer::operator()( const BarzelTrieNode& t ) 
{
	BTN_cp tn = &t;
	BTNDataHash::iterator i = d_dtaHash.find( tn );
	if( i == d_dtaHash.end() ) {
		i = d_dtaHash.insert( BTNDataHash::value_type( tn, TA_BTN_data() ) ).first;
	}
	updateAnalytics( tn, i->second );
	return true;
}

std::ostream& TA_BTN_data::print( std::ostream& fp, const StoredUniverse& u ) const
{
	return fp << numDesc << "," << entities.size() << std::endl;
}
///// name producer 

bool TANameProducer::operator()( TrieAnalyzer& analyzer, const BarzelTrieNode& t ) 
{
	const BarzelTrieNode* tn = &t;
	const TA_BTN_data* dta = analyzer.getTrieNodeData( tn );
	ay::char_cp_vec tokVec;
	std::vector< uint32_t > stringIdVec;

	if( dta ) {
		if( dta->entities.size() <= analyzer.getNameThreshold() ) {
			
			if( isMode_output() && analyzer.getPathTokens( tokVec ) ) {

				if( d_maxNameLen < tokVec.size() )
					return true;
				analyzer.getPathTokens( stringIdVec );


				if( d_fluffTrie.getLongestPath( stringIdVec.begin(), stringIdVec.end()).first ||
					d_fluffTrie.getLongestPath( stringIdVec.rbegin(), stringIdVec.rend()).first )
						return true;

				++d_numNames;
				/// printing names for all entities 
				for( int i = 0; i< dta->entities.size(); ++i ) {
					const StoredEntity* ent = analyzer.getEntityById( dta->entities[i] );
					if( !ent )  // should nev er be the case
						continue;
					const StoredEntityUniqId& euid = ent->euid;
					const char* entIdStr = analyzer.getIdStr( euid );
					d_fp << "NAME[" << dta->entities.size() << "]" << 
					entIdStr << '|' << euid.eclass << '|';
					for( ay::char_cp_vec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
						d_fp << " " << *i;
					}
					d_fp << std::endl;
				}
			}
		} else 
		if( dta->entities.size() >= analyzer.getFluffThreshold() ) {
			analyzer.getPathTokens( stringIdVec );

			if( isMode_output() ) {
				if( analyzer.getPathTokens( tokVec ) ) {
					++d_numFluff;
					// d_universe.getDtaIdx().getNumberOfEntities()
					double pct = (double)(dta->entities.size())*100.0 / (double)(analyzer.getUniverse().getDtaIdx().getNumberOfEntities() +1 );
					d_fp << "FLUFF[" << pct << "]:";
					for( ay::char_cp_vec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
						d_fp << " " << *i;
					}
					d_fp << std::endl;
				}
			} else {
				// analytical mode - adding forward and backward fluff
				d_fluffTrie.addKeyPath( stringIdVec.begin(), stringIdVec.end() );
				d_fluffTrie.addKeyPath( stringIdVec.rbegin(), stringIdVec.rend() );
			}
		}
		//AYDEBUG( dta->entities.size() );
	}
	return true;
}
} // namespace barzer
 
