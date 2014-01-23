#include <barzer_topics.h>

namespace barzer {

void TopicEntLinkage::updateFilterTypePairs( const StoredEntityClass& topicClass, const StoredEntityClass& entClass )
{
    for( const auto& i: d_filterTypePairs ) { if( i.first== topicClass&& i.second== entClass) return; }
    d_filterTypePairs.push_back( std::pair<StoredEntityClass,StoredEntityClass>( topicClass, entClass) );
}
bool TopicEntLinkage::topicTypeAppliesToEntClass( const StoredEntityClass& topicClass, const StoredEntityClass& entClass ) const
{
    for( const auto& i: d_filterTypePairs ) { if( i.first== topicClass && i.second== entClass) return true; }
    return false;
}

void TopicEntLinkage::link( const BarzerEntity& t, const EntityData& e )
{
    BarzerEntitySetMap::iterator i = topicToEnt.find( t );
    if( i == topicToEnt.end() ) 
        i = topicToEnt.insert( BarzerEntitySetMap::value_type( t, BarzerEntitySet() ) ).first; 

    i->second.insert(e);
}

void TopicEntLinkage::link( const BarzerEntity& t, const BarzerEntity& e, uint32_t strength ) 
{
    BarzerEntitySetMap::iterator i = topicToEnt.find( t );
    if( i == topicToEnt.end() ) 
        i = topicToEnt.insert( BarzerEntitySetMap::value_type( t, BarzerEntitySet() ) ).first; 
    i->second.insert( EntityData(e,strength) );
    updateFilterTypePairs( t.eclass, e.eclass );
}

void TopicEntLinkage::append( const TopicEntLinkage& lkg ) 
{
    for( BarzerEntitySetMap::const_iterator i = lkg.topicToEnt.begin(); i!= lkg.topicToEnt.end(); ++i ) {
        const BarzerEntity& topic = i->first;
        const BarzerEntitySet& linkedSet = i->second;
        for( BarzerEntitySet::const_iterator lei = linkedSet.begin(); lei != linkedSet.end(); ++lei ) {
                link( topic, *lei );
        }
    }
}

void TopicEntLinkage::clear() 
{
    topicToEnt.clear();
    d_filterTypePairs.clear(); 
}
    
BarzTopics::EntWeightVec& BarzTopics::computeTopTopics( ) 
{
    if( !d_topics.empty() ) {
        for( BarzTopics::TopicMap::const_iterator i = d_topics.begin(); i!= d_topics.end(); ++i ) {
            d_vec.push_back( *i );
        }
        /// sorting by weight for processing 
        std::sort( d_vec.begin(), d_vec.end() );
        /// now we have a vector sorted in descending order 
        /// we will trim it 
      
        size_t newSize = 1;
        int weight = d_vec.front().second;
        for( ;newSize< d_vec.size(); ++newSize ) {
            if( d_vec[newSize].second < weight ) 
                break;
        }
        if( newSize< d_vec.size() ) {
            d_vec.resize( newSize );
            d_topics.clear();
            for( EntWeightVec::const_iterator i = d_vec.begin(); i!= d_vec.end(); ++i ) {
                d_topics.insert( *i );
            }
        }
    }
    return d_vec;
}


void TrieTopics::mustHave( const BarzerEntity& ent, int minWeight )
{
    d_entWeight.first = ent;
    d_entWeight.second = minWeight;
}

bool TrieTopics::goodToGo( const BarzTopics& barzTopics ) const
{
    if( !d_entWeight.first.eclass.isValid() ) 
        return true;
    else {
        const BarzerEntity& ent = d_entWeight.first;

        int w = barzTopics.getTopicWeight(ent);
        return( w>= d_entWeight.second ) ;
    }
    return false;
}

bool TopicFilter::addFilteredEntClass( const StoredEntityClass& ec )
{
    for( const auto& i : d_filteredEntClasses ) { if( i== ec ) return false; }
    d_filteredEntClasses.push_back( ec );
    return true;
}
bool TopicFilter::isFilterApplicable( const StoredEntityClass& ec ) const 
{
    for( const auto&i : d_filteredEntClasses ){ if( i == ec ) return true; }
    return false;
}
void TopicFilter::clear() 
{ 
    d_filterVec.clear(); 
    d_filteredEntClasses.clear();
}

bool TopicFilter::addTopic( const BarzerEntity& t )
{
    if( std::find( d_filteredEntClasses.begin(), d_filteredEntClasses.end(), t.eclass ) == d_filteredEntClasses.end() ) 
        return false;
    
    for( auto& i : d_filterVec ) {
        if( i.front().first.eclass == t.eclass ) {
            for( auto& p : i ) {
                if( p.first.tokId == t.tokId )
                    return false;
            }
            if( const TopicEntLinkage::BarzerEntitySet* x = d_topicLinks.getTopicEntities(t) )
                return ( i.push_back( { t, x } ), true );
            else
                return false;
        }
    }
    // if we're here means we need to create a brand new TopicEntSetPair
    if( auto x = d_topicLinks.getTopicEntities(t) ) {
        d_filterVec.resize( d_filterVec.size() +1 );
        d_filterVec.back().push_back( {t,x} );
        return true;
    } else
        return false;
}
void TopicFilter::optimize() {
    std::sort( d_filterVec.begin(),d_filterVec.end(), 
        []( const Ent2EntSetVec& l, const Ent2EntSetVec& r ) -> bool {
                size_t ls = 0, rs = 0;
            for( const auto& i : l ) ls+= i.second->size();
            for( const auto& i : r ) rs+= i.second->size();
            return ls/l.size() < rs/r.size();
        }
    );
}
bool TopicFilter::isEntityGood( const BarzerEntity& e ) const 
{
    if( !isFilterApplicable(e.eclass) )
        return true;

    for( const auto& fv: d_filterVec ) {
        bool isGood = false;
        for( const auto& f: fv ) {
            if( f.second->find( e ) != f.second->end() ){
                isGood = true;
                break;
            }
        }
        if( !isGood ) 
            return false;
    }
    return true;
}

} // namespace barzer 
