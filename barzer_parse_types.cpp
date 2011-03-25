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
	else 
		fp << "(nostore)";
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
		std::string tmp( buf, len );
		return (fp << len << ':' << tmp);
	}
}

std::ostream& operator<<( std::ostream& fp, const TTWPVec& v )
{
	for( TTWPVec::const_iterator i = v.begin(); i!= v.end(); ++i ) 
		fp << *i << "\n";
	return fp;
}

void Barz::syncQuestionFromTokens()
{
	size_t qLen = 0;
	for( TTWPVec::const_iterator i = ttVec.begin(); i!= ttVec.end(); ++i ) {
		qLen+= (i->first.len+1);
	}
	if( !qLen ) 
		return ;
	question.clear();
	question.resize( qLen );
	size_t pos = 0;
	for( TTWPVec::iterator i = ttVec.begin(); i!= ttVec.end(); ++i ) {
		TToken& t = i->first;
		char* pBuf = &(question[pos]);
		memcpy( pBuf, t.buf, t.len );
		t.buf = pBuf;
		pBuf[t.len] = 0;
		pos+= (t.len+1);
	}
	
}

int Barz::tokenize( QTokenizer& tokenizer, const char* q, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	puVec.clear();
	ctVec.clear();
	ttVec.clear();

	questionOrig.assign(q);
	int rc = tokenizer.tokenize( ttVec, questionOrig.c_str(), qparm );
	
	syncQuestionFromTokens();
	return rc;
}

int Barz::classifyTokens( QLexParser& lexer, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	puVec.clear();
	ctVec.clear();

	return lexer.lex( ctVec, ttVec, qparm );
}

void Barz::clear()
{
	puVec.clear();
	ctVec.clear();
	ttVec.clear();

	question.clear();
}

int Barz::semanticParse( QSemanticParser& sem, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	puVec.clear();

	return sem.semanticize( puVec, ctVec, qparm );
}

} // barzer namespace 
