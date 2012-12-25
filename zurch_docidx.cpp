#include <zurch_docidx.h>
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

int DocFeatureIndex::serialize( std::ostream& fp ) const
{
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
    fp >> tmp ;
    if( tmp != "BEGIN_IDX" ) 
        return 1;
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
