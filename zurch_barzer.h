#pragma once 

#include <map>
#include <vector>
#include <barzer_entity.h>

namespace barzer{ 
struct StoredEntityClass;
}

/// links zurch and barzer
namespace zurch {

class DocIndexLoaderNamedDocs;

using barzer::BarzerEntity;

class BarzerEntityDocLinkIndex {
	std::map<uint32_t, std::vector<uint32_t>> m_doc2linkedEnts;
public:
    DocIndexLoaderNamedDocs& d_zurchLoader;
    BarzerEntityDocLinkIndex( DocIndexLoaderNamedDocs& loader ): d_zurchLoader(loader) {}

    std::string d_fileName;
    /// when arg = 0 uses d_fileName
    /// otherwise sets it
    /// by default the file is assumed to be pipe delimited  
    int loadFromFile( const std::string& fname );
    void addLink( const BarzerEntity& ent, const std::string& s );

	const std::vector<uint32_t>* getLinkedEnts(uint32_t) const;
};

} // namespace zurch 
