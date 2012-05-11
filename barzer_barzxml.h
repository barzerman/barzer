#ifndef BARZER_BARZXML_H
#define BARZER_BARZXML_H
namespace barzer {

class Barz;
struct BarzXMLParser {
    Barz& barz;
    const StoredUniverse& universe;

    void takeTag( const char* tag, const char** attr, bool open=true );
    void takeCData( const char* dta, const char* dta_len );

    BarzXMLParser( Barz& b, const StoredUniverse& u ) :
        barz(b), universe(u) {}
};

}
#endif // BARZER_BARZXML_H
