#pragma once
#include <barzer_entity.h>
#include <boost/unordered_map.hpp>
#include <boost/property_tree.hpp>
#include <vector>

namespace barzer {
class StoredUniverse;
class GlobalPools;

class Multiverse_BENI_Loader {
    GlobalPools& d_gp;
public:
    typedef boost::unordered_map< uint32_t, std::vector<size_t> > UniverseColMap;
    struct LoadParm {
        char sep; /// sepearator in ascii file
        char nameJoinChar;  // if we're indexing multiple columns in an ascii file as one string, this character will be used to join the columns
            // for EXAMPLE: if nameJoinChar is ';' and example col1 is "foo" , col2 is "bar". then we will incex "foo;bar" 
        size_t col_class, col_subclass, col_id, col_relevance;
        std::string mode;
        const StoredEntityClass dfec, dfTopicEc;
        LoadParm( ): sep('\t'), nameJoinChar(' '), col_class(0xffffffff), col_subclass(0xffffffff), col_id(2), col_relevance(3) {}
    };
    
    Multiverse_BENI_Loader( GlobalPools& gp ) : d_gp(gp) {}
    void addUniverseColumn( uint32_t uid, size_t col );
    void runPropertyTreeNode( const boost::property_tree::ptree& pt, const boost::optional<const ptree&> attrOpt );
    
    size_t loadFromFile( const std::string& fname, const LoadParm& parm, const UniverseColMap& colMap );
};

} // namespace barzer 
