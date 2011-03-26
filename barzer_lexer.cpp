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
	CTWPVec::iterator nBeg = cvec.begin(), nEnd = cvec.begin();
	CTWPVec::iterator i = cvec.begin();
	for( ; i!= cvec.end(); ++i ) {
		CToken& t = i->first;

		bool isMinus = t.isPunct('-');
		bool hasDigits = false;
		if( t.isNumber() || isMinus ) {
			std::stringstream sstr;
			if( isMinus )
				sstr << '-';
			else
				sstr << t.bNum;
			hasDigits = !isMinus;

			CTWPVec::iterator j=i;
			++j;
			bool hasDot = false;
			bool hasFraction = false;
			/// this only assumes simple decimals and regular numbers
			/// if i is a number then first j cant be a number (tokenization 
			/// cant produce 2 non space separated numbers 
			/// so it's either . or non digit and we will need to do anything
			/// only if it's .
			for(; j!= cvec.end(); ++j ) {
				if( j->first.isNumber() ) {
						hasDigits = true;
						if( hasDot )
							hasFraction = true;
						sstr << t.bNum;
				} if( j->first.isPunct('.') ) {
					if( hasDot ) {
						if( hasDigits ) {
							std::string tmp = sstr.str();
							if( hasFraction ) {
								double x = atof( tmp.c_str() );
								t.setNumber( x );
								collapseCTokens( i, ++j );
							} else if( isMinus ) {
								int x = atoi( tmp.c_str() );
								t.setNumber( x );
								collapseCTokens( i, ++j );
							}
						}
						i=j;
						break;
					} else {
						sstr << '.';
					}
				} else {
					if( hasDot ) {
						if( hasDigits ) {
							std::string tmp = sstr.str();
							double x = atof( tmp.c_str() );
							t.setNumber( x );
							collapseCTokens( i, ++j );
						}
					} else if( isMinus ) {
						std::string tmp = sstr.str();
						int x = atoi( tmp.c_str() );
						t.setNumber( x );
						collapseCTokens( i, ++j );
					}
					i=j;
					break;
				}
			}
		} 
	}
	return 0;
}

int QLexParser::advancedBasicClassify( CTWPVec& cvec, const TTWPVec& tvec, const QuestionParm& qparm )
{
	// transforms 
	advancedNumberClassify(cvec,tvec,qparm);
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
	for( s = q; *s; ++s ) {
		char c = *s;
		if( !isascii(c) ) {
			if( !tok ) 
				tok = s;
		} else
		if( isspace(c) ) {
			if( !lc || lc != c ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				ttwp.push_back( 
						TTWPVec::value_type(
							TToken(s,1),
							ttwp.size() 
						)
				);
			}
		} else if( ispunct(c) ) {
			if( tok ) {
				ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
				tok = 0;
			}
			ttwp.push_back( TTWPVec::value_type( TToken(s,1), ttwp.size() ));
		} else if( isalnum(c) ) {
			if( !tok ) 
				tok = s;
		}
	    lc=c;
	}
	if( tok ) {
		ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
		tok = 0;
	}
	return ( ttwp.size() - origSize );
}

} // barzer namespace 
