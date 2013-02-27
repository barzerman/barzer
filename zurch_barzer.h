#pragma once 

#include <barzer_entity.h>
#include <map>

namespace barzer{ 
struct StoredEntityClass;
}

/// links zurch and barzer
namespace zurch {

class DocIndexLoaderNamedDocs;

using barzer::BarzerEntity;

class BarzerEntityDocLinkIndex {
public:
    DocIndexLoaderNamedDocs& d_zurchLoader;
    BarzerEntityDocLinkIndex( DocIndexLoaderNamedDocs& loader ): d_zurchLoader(loader) {}

    std::string d_fileName;
    /// when arg = 0 uses d_fileName
    /// otherwise sets it
    /// by default the file is assumed to be pipe delimited  
    int loadFromFile( const std::string& fname );
    void addLink( const BarzerEntity& ent, const std::string& s );
};

} // namespace zurch 
