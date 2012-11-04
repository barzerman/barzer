#include <barzer_parse_types.h>
#include <barzer_storage_types.h>
#include <barzer_parse.h>
#include <barzer_universe.h>
#include <iomanip>

namespace barzer {
void  CToken::syncStemAndStoredTok(const StoredUniverse& u)
{

    if( storedTok ) {
        if( stemTok && (stemTok == storedTok) && !u.getGlobalPools().getStemSrcs(storedTok->stringId) )
            stemTok= 0;
    } else if( stemTok ) {
        /*
        storedTok = stemTok;
        stemTok = 0;
        */
    }
}

std::ostream& CToken::printQtVec( std::ostream& fp ) const
{
	for( TTWPVec::const_iterator i = qtVec.begin(); i!= qtVec.end(); ++i ) {
		fp << i->second << ":\"" ;
		fp.write( i->first.buf.c_str(), i->first.buf.length() );
		fp <<"\" ";
	}
	return fp;
}
std::ostream& CToken::print( std::ostream& fp ) const
{
	fp << "[" ;
	printQtVec(fp) << "]";

	fp << cInfo << ling;
	if( isNumber() ) {
		fp << "=" << bNum;
	}
	if( storedTok ) 
		fp << " strd[" << *storedTok << "]";
	return fp;
}

std::ostream& operator<<( std::ostream& fp, const CTWPVec& v )
{
	for( CTWPVec::const_iterator i = v.begin(); i!= v.end(); ++i ) {
		fp << i->first << std::endl;
	}
	return fp;
}

// CToken service functions  and output

void CToken::syncClassInfoFromSavedTok()
{
	if( storedTok ) {
		if( storedTok->classInfo.theClass == StoredTokenClassInfo::CLASS_NUMBER ) 
			cInfo.theClass = CTokenClassInfo::CLASS_NUMBER ;
		else
			cInfo.theClass = CTokenClassInfo::CLASS_WORD ;
	}
/// copy settings from storedToken's info sub-object into cInfo
}

/// TToken service functions and output
std::ostream& TToken::print ( std::ostream& fp ) const
{
	if( !buf.length() ) {
		return ( fp << 0 <<':' );
	} else {
        if( glyphsAreValid() ) {
            fp << "GLYPH[" << getFirstGlyph() << " " << getNumGlyphs() << "]:" ;
        } else 
            fp << "NOGLYPH" << ":";
		return ( fp << buf.length() << ':' ).write( buf.c_str(), buf.length() );
	}
}

std::ostream& operator<<( std::ostream& fp, const TTWPVec& v )
{
	for( TTWPVec::const_iterator i = v.begin(); i!= v.end(); ++i ) 
		fp << *i << "\n";
	return fp;
}

void BarzerString::setFromTTokens( const TTWPVec& v )
{
	str.clear();
	/// most of the time there will only be 1 iteration here
	for( TTWPVec::const_iterator i = v.begin(); i!= v.end(); ++i ) {
		const TToken& ttok = i->first;
		str.append( ttok.buf.c_str(), ttok.buf.length() );
	}
}

} // barzer namespace 
