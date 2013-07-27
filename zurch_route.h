#pragma once
#include <string>
#include <iostream>
namespace barzer {
    class BarzerRequestParser;
    struct QuestionParm;
}
namespace zurch {

class DocIndexAndLoader;

class ZurchRoute {
public:
    const DocIndexAndLoader& d_ixl;
    barzer::BarzerRequestParser& d_rqp;
    
    ZurchRoute(const DocIndexAndLoader& ixl, barzer::BarzerRequestParser& rqp ) :
        d_ixl(ixl),
        d_rqp(rqp)
    {}
    int operator()( const char* q );
}; 

} // namespace zurch
