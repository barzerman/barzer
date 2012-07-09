#ifndef BARZER_BARZXML_H
#define BARZER_BARZXML_H

#include <barzer_parse_types.h>
namespace barzer {

class Barz; 
class BarzerRequestParser; 
struct BarzXMLParser {
	bool m_shouldInternStrings;

    Barz& barz;
    BarzerRequestParser& reqParser; 
    QuestionParm qparm;

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

    BarzXMLParser( Barz& b, BarzerRequestParser& rp, const StoredUniverse& u ) :
        m_shouldInternStrings(false), barz(b), reqParser(rp), universe(u) {}

	void setInternStrings(bool);
	bool internStrings() const;
private:
    void setLiteral( BarzelBead&, const char*, size_t, bool isFluff );
};

}
#endif // BARZER_BARZXML_H
