#pragma once 

#include <boost/unordered_map.hpp>
#include <ay/ay_stackvec.h>
#include <vector>
#include <zurch_docidx.h>

namespace zurch {

/// XML format for long FAQ
struct Zurch_Moedelo_Doc_FAQ {
    std::string d_BegDate;
    std::string d_Content;
    std::string d_DocName;
    std::string d_ID;
    std::string d_Keywords;
    std::string d_ModuleID;
    std::string d_SortName;
};

//// meanings XML parser 
struct ZurchLongXMLParser {

    Zurch_Moedelo_Doc_FAQ  d_data;

    std::vector<int> tagStack;
    size_t d_numCallbacks;

    /// tag functions are needed for external connectivity so that this object can be invoked from other XML parsers
    void readFromFile( const char* fname );
    void clear() { tagStack.clear(); }

    virtual int callback();
    ZurchLongXMLParser() : d_numCallbacks(0){}
    virtual ~ZurchLongXMLParser() {}
};


struct ZurchLongXMLParser_DocLoader : public ZurchLongXMLParser {
    DocIndexLoaderNamedDocs& d_loader;
    DocFeatureLoader::DocStats& d_loadStats;
    ZurchLongXMLParser_DocLoader(DocFeatureLoader::DocStats& ds, DocIndexLoaderNamedDocs& ldr) : d_loader(ldr), d_loadStats(ds){}
    virtual int callback();
};
} // namespace barzer 