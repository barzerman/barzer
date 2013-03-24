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
	std::string d_NewKeywords;
	std::string d_Rubrics;
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
    // docid|content (conent up to 16MB)
    void clear() { tagStack.clear(); }

    virtual int callback();
    ZurchLongXMLParser() : d_numCallbacks(0){}
    virtual ~ZurchLongXMLParser() {}
};


struct ZurchLongXMLParser_DocLoader : public ZurchLongXMLParser {
    DocIndexLoaderNamedDocs& d_loader;
    DocFeatureLoader::DocStats& d_loadStats;

    bool d_onlyTitlesAndContents; 

    void setLoadOnlyTitlesAndContents( bool x ) { d_onlyTitlesAndContents = x; }

    ZurchLongXMLParser_DocLoader(DocFeatureLoader::DocStats& ds, DocIndexLoaderNamedDocs& ldr) : 
        d_loader(ldr), 
        d_loadStats(ds),
        d_onlyTitlesAndContents(false)
    {}
    virtual int callback();
};

struct ZurchLongXMLParser_Phraserizer : public ZurchLongXMLParser {
    std::ostream& d_outFP;
    PhraseBreaker d_phraser;

    enum {
        MODE_PHRASER,  // phrases
        MODE_EXTRACTOR // extracts content and titles
    };
    int d_mode;
    ZurchLongXMLParser_Phraserizer( std::ostream& fp ) : 
        d_outFP(fp),
        d_mode(MODE_PHRASER)
    {}

    void setPhraserMode() { d_mode=MODE_PHRASER; }
    void setExtractorMode() { d_mode=MODE_EXTRACTOR; }
    virtual int callback();

    void printCurrentRecord();
};
struct ZurchPhrase_DocLoader {
    DocIndexLoaderNamedDocs& d_loader;
    DocFeatureLoader::DocStats& d_loadStats;

    ZurchPhrase_DocLoader( DocFeatureLoader::DocStats& ds, DocIndexLoaderNamedDocs& ldr ) :
        d_loader(ldr), d_loadStats(ds) 
    {}
    void readPhrasesFromFile( const char* fn );
    void readDocContentFromFile( const char* fn, size_t maxLineLen = 8*1024*1024 );
    /// line is pipe seaprated DOCNAME|TX|PHRASENUM|TEXT
};

} // namespace barzer 
