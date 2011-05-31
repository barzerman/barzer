#include <barzer_lexer.h>
#include <ctype.h>
#include <barzer_dtaindex.h>
#include <sstream>

namespace barzer {
bool QLexParser::tryClassify_number( CToken& ctok, const TToken& ttok ) const
{
	const char* beg = ttok.buf;
	const char* end = ttok.buf+ttok.len;
	bool hasDot = false, hasDigit = false;
	for( const char* s = beg; s!= end; ++s ) {
		switch( *s ) {
		case '.':
			if( hasDot ) return false; else hasDot=true;
			break;
		default: 
			if( !isdigit(*s) ) return false; else hasDigit= true;
			break;
		}
	}
	if( hasDigit ) {
		ctok.setClass( CTokenClassInfo::CLASS_NUMBER );
		if( hasDot ) 
			ctok.bNum.setReal( ttok.buf );
		else 
			ctok.bNum.setInt( ttok.buf );

		return true;
	}
	return false;
}

void QLexParser::collapseCTokens( CTWPVec::iterator beg, CTWPVec::iterator end )
{
	if( beg == end ) 
		return;
	CTWPVec::iterator i = beg;
	CToken& target = beg->first;
	++i;
	for( ; i!= end; ++i ) {
		CToken& t = i->first;
		std::copy(t.qtVec.begin(),t.qtVec.end(),  target.qtVec.end() );
		t.clear();
	}
}

int QLexParser::advancedNumberClassify( CTWPVec& cvec, const TTWPVec& tvec, const QuestionParm& qparm )
{
	//// this only processes positive 
	for( size_t i =0; i< cvec.size(); ++i ) {
		CToken& t = cvec[i].first;
		if( t.isNumber() ) {
			size_t i_dot = i+1, i_frac = i+2;
			if( i_frac < cvec.size() ) {
				CToken& dotTok = cvec[i_dot].first;
				CToken& fracTok = cvec[i_frac].first;
				// tokens preceding T and following frac
				CToken* pastFrac = ( i_frac + 1 < cvec.size() ? &(cvec[i_frac+1].first) : 0 );
				CToken* preT = ( i ? &(cvec[i-1].first) : 0 );

				if( 
					(pastFrac && !(pastFrac->isSpace()) && !pastFrac->isPunct(',') ) ||
					(preT && !(preT->isSpace()) && !preT->isPunct(',') ) )  
				{
					continue;
				}
				if( dotTok.isPunct('.') && fracTok.isNumber() ) {
					// floating point
					std::stringstream sstr;
					sstr << t.bNum << '.' << fracTok.bNum;
					double x = atof( sstr.str().c_str() );
					t.setNumber( x );
					t.qtVec.insert( t.qtVec.end(), dotTok.qtVec.begin(), dotTok.qtVec.end() );
					t.qtVec.insert( t.qtVec.end(), fracTok.qtVec.begin(), fracTok.qtVec.end() );
					dotTok.clear();
					fracTok.clear();
				}
			}
		} 
	}
	return 0;
}
namespace {

void removeBlankCTokens( CTWPVec& cvec )
{
	CTWPVec::iterator d = cvec.begin(), s = cvec.begin();
	while( s!= cvec.end()) {
		if( !s->first.isBlank() ) {
			if( s!= d ) *d = *s;
			++d;
		}
		++s;
	}
	if( d!= cvec.end() ) 
		cvec.resize( d-cvec.begin() );
}

}

int QLexParser::advancedBasicClassify( CTWPVec& cvec, const TTWPVec& tvec, const QuestionParm& qparm )
{
	// transforms 
	advancedNumberClassify(cvec,tvec,qparm);
	removeBlankCTokens( cvec );
	return 0;
}

int QLexParser::singleTokenClassify( CTWPVec& cVec, const TTWPVec& tVec, const QuestionParm& qparm )
{
	cVec.resize( tVec.size() );
	if( !dtaIdx ) 
		return ( err.e = QLPERR_NULL_IDX, 0 );

	for( uint32_t i = 0; i< tVec.size(); ++i ) {
		const TToken& ttok = tVec[i].first;
		const char* t = ttok.buf;
		cVec[i].second = i;
		CToken& ctok = cVec[i].first;
		ctok.setTToken( ttok, i );

		if( !t || !*t || isspace(*t) ) { // this should never happen
			ctok.setClass( CTokenClassInfo::CLASS_SPACE );
			continue;
		}
		const StoredToken* storedTok = dtaIdx->getStoredToken( t );
		if( storedTok ) { /// 
			/// 
			ctok.storedTok = storedTok;
			ctok.syncClassInfoFromSavedTok();
		} else { /// token NOT matched in the data set

			if(ispunct(*t)) {
				ctok.setClass( CTokenClassInfo::CLASS_PUNCTUATION );
			} else if( !tryClassify_number(ctok,t) ) { 
				/// fall thru - this is an unmatched word
				ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
			}
		}
	}
	return 0;
}

int QLexParser::lex( CTWPVec& cVec, const TTWPVec& tVec, const QuestionParm& qparm )
{
	err.clear();
	cVec.clear();
	/// convert every ttoken into a single ctoken 
	singleTokenClassify( cVec, tVec, qparm );
	/// try grouping tokens and matching basic compounded tokens
	/// non language specific
	advancedBasicClassify( cVec, tVec, qparm );

	/// perform advanced language specific lexing 
	langLexer.lex( cVec, tVec, qparm );
	return 0;
}
////////////// TOKENIZE 
///////////////////////////////////
int QTokenizer::tokenize( TTWPVec& ttwp, const char* q, const QuestionParm& qparm  )
{
	err.clear();

	size_t origSize = ttwp.size(); 
	char lc = 0;
	const char* tok = 0;
	const char* s = q;
	enum {
		CHAR_UNKNOWN=0,
		CHAR_ALPHA,
		CHAR_DIGIT,
		CHAR_UTF8
	};
	int prevChar = CHAR_UNKNOWN;
#define PREVCHAR_NOT( t ) ( prevChar && (CHAR_##t != prevChar) )

	for( s = q; *s; ++s ) {
		char c = *s;
		if( !isascii(c) ) {
			if( PREVCHAR_NOT(UTF8) ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				--s;
			} else {
				if( !tok ) 
					tok = s;
			}
			prevChar = CHAR_UTF8;
		} else
		if( isspace(c) ) {
			if( !lc || lc != c ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				ttwp.push_back( TTWPVec::value_type( TToken(s,1), ttwp.size() ));
			}
			prevChar = CHAR_UNKNOWN;
		} else if( ispunct(c) ) {
			if( tok ) {
				ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
				tok = 0;
			}
			ttwp.push_back( TTWPVec::value_type( TToken(s,1), ttwp.size() ));
			prevChar = CHAR_UNKNOWN;
		} else if( isalnum(c) ) {
			if( (isalpha(c) && PREVCHAR_NOT(ALPHA) ) || 
				(isdigit(c) && PREVCHAR_NOT(DIGIT) ) 
			) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				--s;
			} else  {
				if( !tok ) tok = s;
			}
			prevChar =( isdigit(c) ? CHAR_DIGIT : CHAR_ALPHA );
		} else 
			prevChar = CHAR_ALPHA;
	    lc=c;
	}
	if( tok ) {
		ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
		tok = 0;
	}
	return ( ttwp.size() - origSize );
#undef PREVCHAR_NOT
}

} // barzer namespace 
