#include <barzer_parse_types.h>
#include <barzer_storage_types.h>
#include <barzer_parse.h>
#include <iomanip>

namespace barzer {

std::ostream& CToken::printQtVec( std::ostream& fp ) const
{
	for( TTWPVec::const_iterator i = qtVec.begin(); i!= qtVec.end(); ++i ) {
		fp << i->second << ":\"" ;
		fp.write( i->first.buf, i->first.len );
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
	if( !buf || !len ) {
		return ( fp << 0 <<':' );
	} else {
		return ( fp << len << ':' ).write( buf, len );
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
		str.append( ttok.buf, ttok.len );
	}
}

} // barzer namespace 
