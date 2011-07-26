#include <barzer_el_analysis.h>
#include <barzer_el_trie.h>

namespace barzer {

void TrieAnalyzer::setNameThreshold( size_t n )
{
	d_nameThreshold = n;
	// d_nameThreshold = d_universe.getDtaIdx().getNumberOfEntities()/n;
}
void TrieAnalyzer::setFluffThreshold( size_t n ) 
{
	d_fluffThreshold= n;
	// d_fluffThreshold= (1+ d_universe.getDtaIdx().getNumberOfEntities()/n);
}
TrieAnalyzer::TrieAnalyzer( const StoredUniverse& u, const UniverseTrieClusterIterator& trieClusterIter ) :
	d_universe(u),
	d_trav( trieClusterIter.getCurrentTrie().getRoot(), trieClusterIter.getCurrentTrie() ),
	d_trieClusterIter(trieClusterIter),
	d_nameThreshold(2000),
	d_fluffThreshold(200),
	d_absNameThreshold(8)
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
		const BELTrie& trie = d_trieClusterIter.getCurrentTrie();
		const BarzelTranslation* tran = trie.getBarzelTranslation( *tn );
		if( tran ) {
			if( tran->isMkEntSingle() ) {
				dta.addEntity( tran->getId_uint32(), d_universe );
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

void TA_BTN_data::addEntity( uint32_t i, const StoredUniverse& u ) 
{
	entities.insert(i);
	const StoredEntity* ent = u.getDtaIdx().getEntById( i );
	if( ent ) {
		addOneToEclass( ent->getEclass() );
	}
}

std::ostream& TA_BTN_data::print( std::ostream& fp, const StoredUniverse& u ) const
{
	return fp << numDesc << "," << entities.size() << std::endl;
}

namespace {
struct EclassCountPair_less_bysize { 
	bool operator() ( const TA_BTN_data::EclassCountMap::value_type& l, const TA_BTN_data::EclassCountMap::value_type& r ) const 
	{
		if( l.second < r.second ) 
			return true;
		else if( r.second < l.second ) 
			return false;
		else 
			return l.first< r.first;
	}
};

typedef std::set< TA_BTN_data::EclassCountMap::value_type, EclassCountPair_less_bysize > EclassCountPairSet;
} // anon namespace  ends

bool TrieAnalyzer::dtaBelowNameThreshold( TA_BTN_data::EntVecSet& nameableEntities, const TA_BTN_data& dta ) const
{
	size_t maxFullTotal = 1+ (d_universe.getDtaIdx().getNumberOfEntities())/(1+d_nameThreshold);

	if( dta.entities.size() < getAbsNameThreshold() || dta.entities.size() < maxFullTotal ) { 
		/// simple cases - there are very few entities to begin with - either fewer than the absolute nameability
		/// threshold or fewer than the relative name threshold 
		return true;
	} else {
		EclassCountPairSet nameEclassSet;
		size_t totalNameableCount = 0;
		const TA_BTN_data::EclassCountMap& eclassMap = dta.eclassMap;
		for( TA_BTN_data::EclassCountMap::const_iterator i = eclassMap.begin(); i!= eclassMap.end(); ++i ) {
			if( i->second < getAbsNameThreshold() ) {
				nameEclassSet.insert( *i );
				totalNameableCount += i->second;
			}
			else {
				size_t eclassTotal = d_universe.getDtaIdx().getEclassEntCount( i->first );
				size_t maxCnt = 1+eclassTotal/(1+d_nameThreshold);	
				if( i->second< maxCnt ) {
					nameEclassSet.insert( *i );
					totalNameableCount += i->second;
				}
			}
		}
		/// at this point nameEclassSet has pairs (eclass,count) for every eclass in which it's nameable 
		/// 
		if( totalNameableCount >= maxFullTotal ) {
			/// this means there are a few subclasses such that qualifying nameable entity count across all 
			/// of them exceeds the relative nameability threshold
			/// in this case we either have a clear winner or group of winner eclasses or dta is not nameable 
			///  nameEclassSet is ordered by the counts we will try to only leave the winners 
			size_t lastCount = 0, curTotalNameableCount = 0;
			EclassCountPairSet::iterator firstExtraPair = nameEclassSet.begin();
			for(  EclassCountPairSet::iterator i = nameEclassSet.begin(); (curTotalNameableCount< maxFullTotal) && i!= nameEclassSet.end(); ++i ) {

				if( !lastCount ) 
					lastCount = i->second;
				else {
					if( i->second != lastCount ) {
						firstExtraPair = i;
						lastCount = i->second;
					}
				}
				curTotalNameableCount+= i->second;
			}
			if( firstExtraPair == nameEclassSet.begin() ) 
				return false;

			/// at this point nameEclassSet has all eclasses whose entities in dta.entities we will include in the nameable vector
			std::set< StoredEntityClass > validEclassSet;

			/// forming validEclassSet - set of entity classes of interest
			for( EclassCountPairSet::iterator i= nameEclassSet.begin(); i!= firstExtraPair; ++i ) 
				validEclassSet.insert( i->first );

			/// filtering entities by entity classes of interest into nameableEntities
			for( TA_BTN_data::EntVecSet::const_iterator i = dta.entities.begin(); i!= dta.entities.end(); ++i ) {
				const StoredEntity* ent = d_universe.getDtaIdx().getEntById( *i );
				if( validEclassSet.find( ent->getEclass() ) != validEclassSet.end() ) 
					nameableEntities.insert( *i );
			}
			return true;
		}
	}
	return false;
}

bool TrieAnalyzer::dtaAboveFluffThreshold( const TA_BTN_data& dta ) const
{
	for( TA_BTN_data::EclassCountMap::const_iterator i = dta.eclassMap.begin(); i!= dta.eclassMap.end(); ++i ) {
		if( i->second < getAbsNameThreshold() ) {
			return false;
		}
		else {
			size_t eclassTotal = d_universe.getDtaIdx().getEclassEntCount( i->first );
			size_t maxCnt = 1+eclassTotal/(1+d_nameThreshold);	
			if( i->second< maxCnt ) 
				return false;
		}
	}
	return isOverThreshold(dta.entities.size(),getTotalEntCount(),getFluffThreshold()) ;
}
///// name producer 

bool TANameProducer::operator()( TrieAnalyzer& analyzer, const BarzelTrieNode& t ) 
{
	const BarzelTrieNode* tn = &t;
	const TA_BTN_data* dta = analyzer.getTrieNodeData( tn );
	ay::char_cp_vec tokVec;
	std::vector< uint32_t > stringIdVec;
	
	TA_BTN_data::EntVecSet nameableEntities;

	if( dta ) {
		const TA_BTN_data::EntVecSet* entitiesP = 0;
			

		if( dta->entities.size() < analyzer.getAbsNameThreshold() || 
			analyzer.isUnderThreshold(dta->entities.size(),analyzer.getTotalEntCount(),analyzer.getNameThreshold()) 
		) 
			entitiesP = &(dta->entities);
		else {
			nameableEntities.clear();
			if( analyzer.dtaBelowNameThreshold( nameableEntities, *dta ) ) 
				entitiesP = &nameableEntities;
		}
		if( entitiesP ) {
			const TA_BTN_data::EntVecSet& entities = *entitiesP;	

			if( isMode_output() && analyzer.getPathTokens( tokVec ) ) {

				if( d_maxNameLen < tokVec.size() )
					return true;
				analyzer.getPathTokens( stringIdVec );
				if( d_fluffTrie.getLongestPath( stringIdVec.begin(), stringIdVec.end()).first ||
					d_fluffTrie.getLongestPath( stringIdVec.rbegin(), stringIdVec.rend()).first )
						return true;

				++d_numNames;
				/// printing names for all entities 
				for( size_t i = 0; i< entities.size(); ++i ) {
					const StoredEntity* ent = analyzer.getEntityById( entities[i] );
					if( !ent )  // should nev er be the case
						continue;
					const StoredEntityUniqId& euid = ent->euid;
					const char* entIdStr = analyzer.getIdStr( euid );
					d_fp << "NAME[" << entities.size() << "]" << 
					entIdStr << '|' << euid.eclass << '|';
					for( ay::char_cp_vec::const_iterator j = tokVec.begin(); j!= tokVec.end(); ++j ) {
						d_fp << *j << " ";
					}
					d_fp << std::endl;
				}
			}
		} else 
		if( analyzer.dtaAboveFluffThreshold(*dta) ) {
			analyzer.getPathTokens( stringIdVec );
			analyzer.getPathTokens( tokVec );

			if( isMode_output() ) {
				++d_numFluff;
				// d_universe.getDtaIdx().getNumberOfEntities()
				double pct = (double)(dta->entities.size())*100.0 / (double)(analyzer.getUniverse().getDtaIdx().getNumberOfEntities() +1 );
				d_fp << "FLUFF[" << pct << "]:";
				for( ay::char_cp_vec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
					d_fp <<  *i << " ";
				}
				d_fp << std::endl;
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
 
