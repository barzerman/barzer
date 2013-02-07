#pragma once 

#include <boost/unordered_map.hpp>
#include <ay/ay_stackvec.h>
#include <vector>

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

    /// tag functions are needed for external connectivity so that this object can be invoked from other XML parsers
    void readFromFile( const char* fname );
    void clear() { tagStack.clear(); }
};

} // namespace barzer 
