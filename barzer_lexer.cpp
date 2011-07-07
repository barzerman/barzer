#include <barzer_lexer.h>
#include <ctype.h>
#include <barzer_dtaindex.h>
#include <barzer_universe.h>
#include <sstream>
#include <ay_char.h>

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
			ctok.number().setReal( ttok.buf );
		else  {
			ctok.number().setInt( ttok.buf );
			ctok.number().setAsciiLen( ttok.len );
		}
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
					sstr << t.number() << '.' << fracTok.number();
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

int QLexParser::trySpellCorrectAndClassify( CToken& ctok, TToken& ttok )
{
	const char* t = ttok.buf;

	bool isAsciiToken = ttok.isAscii();

	if( isAsciiToken && ttok.len > MIN_SPELL_CORRECT_LEN ) {
		BarzerHunspellInvoke spellChecker(d_universe.getHunspell(),d_universe.getGlobalPools());
		ay::LevenshteinEditDistance& editDist = spellChecker.getEditDistanceCalc();
		std::pair< int, size_t> scResult = spellChecker.checkSpell( t );
		
		const char*const* sugg = 0;
		size_t sugg_sz = 0;
		if( !scResult.first ) { // this is a misspelling 
			sugg = spellChecker.getAllSuggestions();
			sugg_sz = scResult.second;
		}

		const char* bestSugg = 0;
		for( size_t i =0; i< sugg_sz; ++i ) {
			const char* curSugg = sugg[i];
			if( editDist.ascii_no_case( t, curSugg ) <= MAX_EDIT_DIST_FROM_SPELL) {
				
				/// current suggestion contains at least one space which means
				/// the spellchecker has broken up the input 
				if( strchr( curSugg, ' ' ) ) { }
				else { // spell corrected to a solid token
					// if this suggeston resolves to a token we simply return it
					// we should also take care of word split spelling correction
					const StoredToken* storedTok = dtaIdx->getStoredToken( curSugg );
					if( storedTok ) { /// 
						/// 
						ctok.storedTok = storedTok;
						ctok.syncClassInfoFromSavedTok();
						ctok.addSpellingCorrection( t, curSugg );
						return 0;
					} else
						bestSugg = curSugg;
				}
			}
		}
		// nothing was resolved/spell corrected so we will try stemming
		{
		BarzerHunspellInvoke stemmer(d_universe.getHunspell(),d_universe.getGlobalPools());
		const char* stem = stemmer.stem( t );
		const StoredToken* storedTok = dtaIdx->getStoredToken(stem);
		if( storedTok ) {
			/// 
			ctok.storedTok = storedTok;
			ctok.syncClassInfoFromSavedTok();
			ctok.addSpellingCorrection( t, stem );
			return 1;
		}
		} // end of block
	} else {
		/// non-ascii spell checking logic should go here 
	}


	ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
	return -1;
}

int QLexParser::singleTokenClassify( CTWPVec& cVec, TTWPVec& tVec, const QuestionParm& qparm )
{
	cVec.resize( tVec.size() );
	if( !dtaIdx ) 
		return ( err.e = QLPERR_NULL_IDX, 0 );
	
	for( uint32_t i = 0; i< tVec.size(); ++i ) {
		TToken& ttok = tVec[i].first;
		const char* t = ttok.buf;
		cVec[i].second = i;
		CToken& ctok = cVec[i].first;
		ctok.setTToken( ttok, i );
		bool wasStemmed = false;

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

				/// lets try to spell correct the token
				if( d_universe.stemByDefault() ) {
					if( trySpellCorrectAndClassify( ctok, ttok ) > 0 )
						wasStemmed = true;
	 			} else {
					ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
				}
			}
		}
		/// stemming 
		if( ctok.isWord() && d_universe.stemByDefault() && !wasStemmed ) {
			BarzerHunspellInvoke spellChecker(d_universe.getHunspell(),d_universe.getGlobalPools());
			std::string strToStem( ttok.buf, ttok.len );
			const char* stem = spellChecker.stem( strToStem.c_str() );
			if( stem )
				ctok.stemTok = dtaIdx->getStoredToken( stem );
		}
	}

	return 0;
}

int QLexParser::lex( CTWPVec& cVec, TTWPVec& tVec, const QuestionParm& qparm )
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
