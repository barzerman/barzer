#ifndef BARZER_BARZXML_H
#define BARZER_BARZXML_H

namespace barzer {

class Barz; 
struct BarzXMLParser {
    Barz& barz;
    
    // current stack of tags - see .cpp file for tag codes 
    std::vector< int > tagStack;

    const StoredUniverse& universe;
    
    void takeTag( const char* tag, const char** attr, size_t attr_sz, bool open=true );
    void takeCData( const char* dta, size_t dta_len );
    
    bool isCurTag( int tid ) const
        { return ( tagStack.back() == tid ); }
    bool isParentTag( int tid ) const
        { 
            return ( tagStack.size() > 1 && (*(tagStack.rbegin()+1)) == tid );
        }

    BarzXMLParser( Barz& b, const StoredUniverse& u ) :
        barz(b), universe(u) {}
private:
    void setLiteral( BarzelBead&, const char*, size_t, bool isFluff );
};

}
#endif // BARZER_BARZXML_H
