#include <zurch_docidx.h>
#include <barzer_universe.h>
namespace zurch {

class DocFeatureIndexHeuristics {

};

/// feature we keep track off (can be an entity or a token - potentially we will add more classes to it)
int DocFeature::serialize( std::ostream& fp ) const
{
    fp << ( featureClass == CLASS_ENTITY ? "e" : "t" ) << " " << std::hex << featureId;
    return 0;
}
int DocFeature::deserialize( std::istream& fp )
{
    char s;
    fp >> s;
    if( s== 'e' )
        featureClass = CLASS_ENTITY;
    else 
        featureClass = CLASS_TOKEN;
    fp >> std::hex >> featureId;
    return 0;
}

//// position of feature in the document
int FeatureDocPosition::serialize( std::ostream& fp ) const
{
    fp << std::dec << weight << " " << offset.first << " " << offset.second;
    return 0;
}
int FeatureDocPosition::deserialize( std::istream& fp )
{
    fp >> std::dec >> weight >> offset.first >> offset.second;
    return 0;
}
/// every document is a vector of ExtractedDocFeature 
int ExtractedDocFeature::serialize( std::ostream& fp ) const
{
    feature.serialize(fp);
    docPos.serialize(fp << ",");
    return 0;
}
int ExtractedDocFeature::deserialize( std::istream& fp )
{
    feature.deserialize( fp );
    char c;
    fp>> c;
    if( c!= ',' ) 
        return 1;
    docPos.deserialize(fp);

    return 0;
}

////  ann array of DocFeatureLink's is stored for every feature in the corpus 
int DocFeatureLink::serialize( std::ostream& fp ) const
{
    fp << std::hex << docId << " " << std::dec << weight << " " << position ;
    return 0;
}
int DocFeatureLink::deserialize( std::istream& fp )
{
    char c;
    fp >> std::hex >> docId >> c >> std::dec >> weight >> c >> position ;
    return 0;
}

DocFeatureIndex::DocFeatureIndex() : d_heuristics( new DocFeatureIndexHeuristics ) { }
DocFeatureIndex::~DocFeatureIndex() { delete d_heuristics; }

/// given an entity from the univere returns internal representation of the entity 
/// if it can be found and null entity (isValid() == false) otherwise
barzer::BarzerEntity DocFeatureIndex::translateExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u ) const
{
    barzer::BarzerEntity newEnt;
    newEnt.eclass = ent.eclass;
    if( const char* s = u.getEntIdString(ent) ) 
        newEnt.tokId = d_stringPool.getId( s );
    
    return newEnt;
}
/// place external entity into the pool (add all relevant strings to pool as well)
uint32_t DocFeatureIndex::storeExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u )
{
    barzer::BarzerEntity newEnt;
    newEnt.eclass = ent.eclass;
    if( const char* s = u.getEntIdString(ent) ) 
        newEnt.tokId = d_stringPool.internIt(s);
    return d_entPool.produceIdByObj( newEnt );
}

uint32_t DocFeatureIndex::storeExternalString( const char* s, const barzer::StoredUniverse& u )
{
    return d_stringPool.internIt(s);
}

uint32_t DocFeatureIndex::storeExternalString( const barzer::BarzerLiteral& l, const barzer::StoredUniverse& u )
{
    if( const char* s = l.toString(u).first ) 
        return d_stringPool.internIt(s);
    else 
        return 0xffffffff;
}
uint32_t DocFeatureIndex::resolveExternalString( const barzer::BarzerLiteral& l, const barzer::StoredUniverse& u ) const
{
    if( const char* s = l.toString(u).first ) 
        return d_stringPool.getId(s);
    else
        return 0xffffffff;
}

void DocFeatureIndex::appendDocument( uint32_t docId, const barzer::Barz& barz )
{
    #warning implement DocFeatureIndex::appendDocument
    for( auto i = barz.getBeadList().begin(); i!= barz.getBeadList().end(); ++i ) {
        if( const barzer::BarzerLiteral* x = i->get<barzer::BarzerLiteral>() ) {
        } else if( const barzer::BarzerEntity* x = i->getEntity() ) {
           
        } else if( const barzer::BarzerEntityList* x = i->getEntityList() ) {
        }
    }
}

void DocFeatureIndex::appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t& v )
{
    for( auto i = v.begin(); i!= v.end(); ++i ) {
        const ExtractedDocFeature& f = *i;
        InvertedIdx_t::iterator  fi = d_invertedIdx.find( f.feature );
        if( fi == d_invertedIdx.end() ) 
            fi = d_invertedIdx.insert( std::pair<DocFeature,DocFeatureLink::Vec_t>( f.feature, DocFeatureLink::Vec_t())).first;
        
        fi->second.push_back( DocFeatureLink(docId, f.weight(), f.simplePosition()) ) ;
    }
}
/// should be called after the last doc has been appended . 
void DocFeatureIndex::sortAll()
{
    for( InvertedIdx_t::iterator i = d_invertedIdx.begin(); i!= d_invertedIdx.end(); ++i ) 
        std::sort( i->second.begin(), i->second.end() ) ;
}

void DocFeatureIndex::findDocument( DocFeatureIndex::DocWithScoreVec_t&, const ExtractedDocFeature::Vec_t& f )
{
#warning implement DocFeatureIndex::findDocument
}

namespace {

struct EntSerializer {
    char buf[256]; // these are tiny entities
    bool operator()( size_t& sz, std::istream& fp ) {
        fp.getline( buf, sizeof(buf), '\n' );
        buf[ sizeof(buf)-1 ] = 0;
        sz = atoi( buf ) ;
        return true;
    }
    bool operator()( barzer::BarzerEntity& ent, std::istream& fp ) 
        { fp >> std::dec >> ent.eclass.ec >> ent.eclass.subclass >> std::hex >> ent.tokId; return true; }
    bool operator()( std::ostream& fp, size_t sz ) const 
        { fp << sz << '\n'; return true; }
    bool operator()( std::ostream& fp, const barzer::BarzerEntity& ent ) const 
        { fp << std::dec << ent.eclass.ec << ent.eclass.subclass << std::hex << ent.tokId; return true;}

};
} // anonymous namespace 
int DocFeatureIndex::serialize( std::ostream& fp ) const
{
    fp << "BEGIN_STRINGPOOL\n";
    d_stringPool.serialize(fp);
    fp << "END_STRINGPOOL\n";

    fp << "BEGIN_ENTPOOL\n";
    {
    EntSerializer srlzr;
    d_entPool.serialize(srlzr,fp);
    }
    fp << "END_ENTPOOL\n";

    /// inverted index
    fp << "BEGIN_IDX\n";
    for( auto i = d_invertedIdx.begin(); i!= d_invertedIdx.end(); ++i ) {
        i->first.serialize( fp << "F " << std::dec << i->second.size() << " ");
        fp << " [ ";
        /// serializing the vector for this feature
        const DocFeatureLink::Vec_t& vec = i->second; 
        for( auto j = vec.begin(); j!= vec.end(); ++j ) {
            if( j!= vec.begin() ) 
                fp << "\n";
            j->serialize(fp);
        }
        fp << " ]";
    }
    fp << "END_IDX\n";
    return 0;
}
int DocFeatureIndex::deserialize( std::istream& fp )
{
    std::string tmp;
    if( !(fp>> tmp) || tmp != "BEGIN_STRINGPOOL" ) return 1;
    d_stringPool.deserialize(fp);
    if( !(fp>> tmp) || tmp != "END_STRINGPOOL" ) return 1;

    if( !(fp>> tmp) || tmp != "BEGIN_ENTPOOL" ) return 1;
    {
    EntSerializer entSerializer;
    d_entPool.deserialize(entSerializer,fp);
    }
    if( !(fp>> tmp) || tmp != "END_ENTPOOL" ) return 1;

    if( !(fp>> tmp) || tmp != "BEGIN_IDX" ) return 1;
    size_t errCount = 0;
    DocFeature feature;
    while( fp >> tmp ) {
        if( tmp== "END_IDX" ) 
            break;
        if( tmp =="]" )  /// end of feature links vec ends (end of feature as well)
            continue;

        if( tmp =="[" ){ /// feature links vec starts
            
        } else if( tmp =="F" ) {  /// feature begins
            size_t sz = 0;
            fp >> sz ;
            if( !sz  ) {
                ++errCount;
                continue;
            }
            if( feature.deserialize( fp ) || !feature.isValid() ) {
                ++errCount;
                continue;
            }
            InvertedIdx_t::iterator fi = d_invertedIdx.find(feature);
            if( fi == d_invertedIdx.end() ) {
                fi = d_invertedIdx.insert( 
                    std::pair<DocFeature,DocFeatureLink::Vec_t>(
                        feature,
                        DocFeatureLink::Vec_t()
                    )
                ).first;
            }
            /// this could simply reserve sz but in case this is an artificially produced file with the same feature 
            /// spread across 
            size_t oldSz = fi->second.size();
            fi->second.resize( oldSz + sz );
            for( auto i = fi->second.begin()+oldSz, end = fi->second.end(); i!= end; ++i ) 
                i->deserialize( fp );
        }
    }
    return errCount;
}

std::ostream& DocFeatureIndex::printStats( std::ostream& fp ) const 
{
    return fp << d_invertedIdx.size();
}

} // namespace zurch
