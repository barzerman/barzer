#include <zurch_classifier.h>

namespace zurch {

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


} // namespace zurch
