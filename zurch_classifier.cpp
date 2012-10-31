#include <zurch_classifier.h>

namespace zurch {

int FeatureExtractor::extractFeatures( ExtractedFeatureMap& efmap, const FeatureExtractor_Base::StringVec &ntVec, bool learnMode )
{
    for( FeatureExtractor_Base::StringVec::const_iterator i = ntVec.begin(); i!= ntVec.end(); ++i ) {
        uint32_t    featureId       = d_tokPool.internIt( i->c_str() );
        
        ExtractedFeatureMap::iterator emi = efmap.find(featureId);
        if( emi == efmap.end() ) 
            emi = efmap.insert( ExtractedFeatureMap::value_type(featureId,0)).first;

        emi->second++;
    }

    normalizeFeatures( efmap, ntVec.size() );
    return 0;
}
int FeatureExtractor::extractFeatures( ExtractedFeatureMap& efmap, const char* buf, bool learnMode )
{
    zurch::ZurchTokenVec tokVec;
    size_t buf_sz = strlen(buf);
    d_tokenizer.tokenize( tokVec, buf, buf_sz );
    if( !tokVec.size() ) 
        return 0;
    for( auto i = tokVec.begin(); i!= tokVec.end(); ++i ) {
        const       ZurchToken& zt  = *i;
        uint32_t    featureId       = d_tokPool.internIt( zt.str.c_str() );
        
        ExtractedFeatureMap::iterator emi = efmap.find(featureId);
        if( emi == efmap.end() ) 
            emi = efmap.insert( ExtractedFeatureMap::value_type(featureId,0)).first;

        emi->second++;
    }

    normalizeFeatures( efmap, tokVec.size() );
    return 0;
}

int FeatureExtractor_Normalizing::extractFeatures( ExtractedFeatureMap& efmap, const FeatureExtractor_Base::StringVec &ntVec, bool learnMode )
{
    for( FeatureExtractor_Base::StringVec::const_iterator i = ntVec.begin(); i!= ntVec.end(); ++i ) {
        uint32_t    featureId       = d_tokPool.internIt( i->c_str() );
        
        ExtractedFeatureMap::iterator emi = efmap.find(featureId);
        if( emi == efmap.end() ) 
            emi = efmap.insert( ExtractedFeatureMap::value_type(featureId,0)).first;

        emi->second++;
    }

    normalizeFeatures( efmap, ntVec.size() );
    return 0;
}
int FeatureExtractor_Normalizing::extractFeatures( ExtractedFeatureMap& efmap, const char* buf_orig, bool learnMode )
{
    zurch::ZurchTokenVec tokVec;
    size_t buf_sz = strlen(buf_orig);

    char* buf= new char[ buf_sz +1 ];
    std::auto_ptr<char> raii( buf );

    memcpy( buf, buf_orig, buf_sz );
    buf[buf_sz]=0;
    int lang = barzer::Lang::getLangNoUniverse( buf, buf_sz );
    barzer::Lang::stringToLower( buf, buf_sz, lang );

    d_tokenizer.tokenize( tokVec, buf, buf_sz );
    if( !tokVec.size() ) 
        return 0;

    ZurchWordNormalizer::NormalizerEnvironment normEnv;
    std::string norm;
    for( auto i = tokVec.begin(); i!= tokVec.end(); ++i ) {
        const       ZurchToken& zt  = *i;
        const char* z_str = zt.str.c_str();
        size_t      z_str_sz;
        uint32_t    featureId       = d_tokPool.getId( zt.str.c_str() );
        if( featureId == 0xffffffff ) { /// may need to transform
            norm.clear();
            d_normalizer.normalize(norm,z_str,normEnv);
            featureId=d_tokPool.internIt(norm.c_str());
        }
        ExtractedFeatureMap::iterator emi = efmap.find(featureId);
        if( emi == efmap.end() ) 
            emi = efmap.insert( ExtractedFeatureMap::value_type(featureId,0)).first;

        emi->second++;
    }

    normalizeFeatures( efmap, tokVec.size() );
    return 0;
}

    std::ostream& DocSetStats::printDeep( std::ostream& fp, const PrintContext& ctxt ) const
    {
        for( auto i = tagStats.begin(); i!= tagStats.end(); ++i ) {
            i->second.printDeep(fp,ctxt) << std::endl;
        }
        return fp;
    }
    std::ostream& DocSetStats::print( std::ostream& fp ) const
    {
        for( auto i = tagStats.begin(); i!= tagStats.end(); ++i ) {
            fp << "tag[" << i->first <<  "]";
            i->second.print(fp) << std::endl;
        }
        return fp;
    }
std::ostream& DocClassifier::print( std::ostream& fp, const PrintContext& ctxt )  const
{
    d_stats.printDeep( fp, ctxt );
    return fp;
}



double TagStats::distance( const ExtractedFeatureMap& doc, const Map& centroid )
{
    const double stdDevEpsilon = 1.0/centroid.size();
        double cumulative = 0.0;
    for( TagStats::Map::const_iterator ci = centroid.begin(), centroid_end = centroid.end(); ci != centroid_end; ++ci ) {
        if( !ci->second.ignore ) {
            ExtractedFeatureMap::const_iterator di = doc.find(ci->second.featureId);

            double x = ( di == doc.end() ? ci->second.mean : (di->second - ci->second.mean) );
            cumulative += (x*x)/(stdDevEpsilon+ci->second.stdDev);
        }
    }
    return sqrt(cumulative);
}
void TagStats::zero() 
{
    for( auto i = hasMap.begin();i!= hasMap.end(); ++i ) 
        i->second.zero();
    for( auto i = hasNotMap.begin();i!= hasNotMap.end(); ++i ) 
        i->second.zero();
}
void TagStats::setIgnore( double epsilon) 
{
    Map newHas, newHasNot;

    for( Map::iterator i = hasMap.begin(); i!=  hasMap.end(); ++i ) {
        Map::iterator not_i = hasNotMap.find(i->first);
        if( not_i != hasNotMap.end() ) {
            if( !i->second.equals( not_i->second, epsilon ) ) 
                continue;
            newHasNot.insert( newHasNot.end(), *i );
        }
        newHas.insert( newHas.end(), *i );
    }
    hasMap.swap(newHas);
    hasNotMap.swap(newHasNot);
}

void TagStats::init( const TagStatsAccumulator& x ) 
{
    hasMap.clear();
    hasNotMap.clear();

    for( const auto& i : x.has.featureMap() ) {
        FeatureStats fs;
        fs.init( i.first, i.second);
        hasMap.insert( std::make_pair(i.first, fs) );
    }
    for( const auto& i : x.hasNot.featureMap() ) {
        FeatureStats fs;
        fs.init( i.first, i.second);
        hasNotMap.insert( std::make_pair(i.first, fs) );
    }
    hasCount = x.hasCount;
    hasNotCount = x.hasNotCount;
}
/// ZurchTrainerAndClassifier
ZurchTrainerAndClassifier::~ZurchTrainerAndClassifier()
{
    clear();
}

void ZurchTrainerAndClassifier::init( extractor_type_t t, const barzer::BZSpell* spell )
{
    clear();
    switch( t ) {
    case EXTRACTOR_TYPE_NORMALIZING:
        extractor = new FeatureExtractor_Normalizing(stringPool,tokenizer,spell);
        break;
    case EXTRACTOR_TYPE_BASIC:
    default:
        extractor = new FeatureExtractor(stringPool,tokenizer);
        break;
    }
    classifier = new DocClassifier(*extractor);
    accumulator = new DocSetStatsAccumulator();
    trainer     = new DataSetTrainer( *extractor,*accumulator );
}
void ZurchTrainerAndClassifier::clear()
{
    delete trainer;
    trainer = 0;
    delete accumulator;
    accumulator = 0;
    delete classifier;
    classifier = 0;
    delete extractor;
    extractor = 0;
}
std::ostream& ZurchTrainerAndClassifier::print( std::ostream& fp, const PrintContext& ctxt ) const
{
    if( trainer ) trainer->print( fp << "TRAINER {", ctxt ) << "}" << std::endl;
    else { fp << "TRAINER IS EMPTY" << std::endl; }
    
    if( classifier ) {
        classifier->print( (fp<< "CLASSIFIER {"), ctxt ) << "}" <<std::endl;
    } 
    else { fp << "CLASSIFIER IS EMPTY" << std::endl; }
    return fp;
}
//// TagStatsAccumulator
void TagStatsAccumulator::accumulate( const ExtractedFeatureMap& f, bool hasTag, double docWeight )
{
    for( auto i = f.begin(); i!= f.end(); ++i ) {
        size_t idx = i->first;
        featureIdMap.insert( idx );
        if( hasTag ) {
            has( idx, i->second, docWeight );
            ++hasCount;
        } else {
            hasNot( idx, i->second, docWeight );
            ++hasNotCount;
        }
    }
}
/// end of TagStatsAccumulator
std::ostream& TagStats::print( std::ostream& fp ) const
{
    return(
        fp << "hasMap:" << hasMap.size() <<  ",hasNotMap:" << hasNotMap.size() << ",hasCount:" << hasCount << ",hasNotCount:" << hasNotCount 
    );
}
std::ostream& TagStats::printDeep( std::ostream& fp, const PrintContext& ctxt ) const
{
    printFeatureStatsMap( fp << "hasMap[" << hasMap.size()  << "]{", hasMap, ctxt ) << "}" << std::endl;
    printFeatureStatsMap( fp << "hasNotMap[" << hasNotMap.size()  << "]{", hasNotMap, ctxt ) << "}" << std::endl;
    return fp;
}
std::ostream& TagStats::printFeatureStatsMap( std::ostream& fp, const Map& m, const PrintContext& ctxt ) const 
{
    for( auto i = m.begin(); i != m.end(); ++i ) {
        fp << i->first << ":";
        ctxt.printStringById(fp,i->first) << std::endl;
        fp << ":" << "(" << i->second << ")";
    }
    return fp;
}
std::ostream& FeatureStats::print( std::ostream& fp) const
{
    return (fp << featureId << ":" << mean << ":" << stdDev);
}
std::ostream& FeatureStats::print( std::ostream& fp, const PrintContext& ) const
{
    return (fp << featureId << ":" << mean << ":" << stdDev);
}

std::ostream& PrintContext::printStringById( std::ostream& fp, uint32_t i ) const
{
    const char* s = ( stringPool? stringPool->printableStr( i ) : 0 );
    if( s && *s ) {
        return fp << s;
    } else {
        return fp << i;
    }
}

std::ostream& DocSetStatsAccumulator::printTagStatsAccumulation( std::ostream& fp, uint32_t tagId, const PrintContext& ctxt ) const
{
    return fp;
}
std::ostream& TagStatsAccumulator::print( std::ostream& fp, const PrintContext& ctxt ) const
{
    return fp;
}
std::ostream& DocSetStatsAccumulator::print( std::ostream& fp, const PrintContext& mode ) const
{
    for( TagStatsAccumulatorMap::const_iterator i = d_tagStats.begin(); i!= d_tagStats.end(); ++i ) {
        fp << "tag[" << i->first << "] {";
        i->second.print( fp, mode );
        fp << "}";
    }
    return fp;
}
std::ostream& DataSetTrainer::print( std::ostream& fp, const PrintContext & ctxt ) const
{
    d_acc.print( fp, ctxt );
    return fp;
}

} // namespace zurch
