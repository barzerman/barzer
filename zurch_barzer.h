#pragma once 

#include <map>
#include <vector>
#include <barzer_entity_basic.h>

namespace barzer{ 
struct StoredEntityClass;
}

/// links zurch and barzer
namespace zurch {

class DocIndexLoaderNamedDocs;

using barzer::BarzerEntity;

class BarzerEntityDocLinkIndex {
public:
    typedef std::pair<uint32_t,float> DocIdData;
private:
	std::map<uint32_t, std::vector<DocIdData> > m_doc2linkedEnts;
public:
    DocIndexLoaderNamedDocs& d_zurchLoader;
    BarzerEntityDocLinkIndex( DocIndexLoaderNamedDocs& loader ): d_zurchLoader(loader) {}

    std::string d_fileName;
    /// when arg = 0 uses d_fileName
    /// otherwise sets it
    /// by default the file is assumed to be pipe delimited  
    int loadFromFile( const std::string& fname );

    void addLink( const BarzerEntity& ent, uint32_t docId, float w );
    void addLink( const BarzerEntity& ent, const std::string& s, float w);

	const std::vector<DocIdData>* getLinkedEnts(uint32_t) const;
};

} // namespace zurch 
