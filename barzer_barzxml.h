#pragma once
#include <barzer_parse_types.h>
namespace barzer {

class Barz; 
struct BarzXMLParser {
	bool m_shouldInternStrings;
	bool m_performCleanup;

    Barz& barz;
	std::ostream& ostream;
	const GlobalPools& gpools;
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

    GlobalPools& getGP_nonconst() { return *(const_cast<GlobalPools*>(&gpools)); }
	BarzXMLParser( Barz& b, std::ostream& os, const GlobalPools& gp, const StoredUniverse& u )
	: m_shouldInternStrings(false)
	, m_performCleanup(true)
	, barz(b)
	, ostream(os)
	, gpools(gp)
	, universe(u)
	{
	}

	void setInternStrings(bool);
	bool internStrings() const;
	
	void setPerformCleanup(bool);
	bool shouldPerformCleanup() const;
private:
    void setLiteral( BarzelBead&, const char*, size_t, bool isFluff );
};

} // namespace barzer

