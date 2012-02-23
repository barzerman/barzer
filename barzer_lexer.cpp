#include <barzer_lexer.h>
#include <ctype.h>
#include <barzer_dtaindex.h>
#include <barzer_universe.h>
#include <sstream>
#include <ay_char.h>
#include <barzer_barz.h>

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
bool QLexParser::tryClassify_integer( CToken& ctok, const TToken& ttok ) const
{
	const char* beg = ttok.buf;
	const char* end = ttok.buf+ttok.len;
	for( const char* s = beg; s!= end; ++s ) {
		if( *s < '0' || *s > '9' )
			return false;
	}
	ctok.setClass( CTokenClassInfo::CLASS_NUMBER );
	ctok.number().setInt( ttok.buf );
	ctok.number().setAsciiLen( ttok.len );
	return true;
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

namespace {

/// ctok is a number
inline bool ctok_number_can_be_in_commadelim( bool isFirst, const CToken& ctok )
{
	if( ctok.getTTokens().size() > 1 || !ctok.getNumber().isInt() )
		return false;
	const TToken*  ttok = ctok.getFirstTToken();
	if( !ttok || ttok->len > 3 )
		return false;

	return (isFirst || ttok->len == 3);
}

inline int64_t get_decimal_factor( int numZeroBlocks )
{
	switch(numZeroBlocks) {
	case 1: return 1000;
	case 2: return 1000000;
	case 3: return 1000000000;
	case 4: return 1000000000000;
	case 5: return 1000000000000000;
	default: return 1;
	}
}

// returns true if this is a comma delimited integer
// endPos will contain the last token of the number
bool is_comma_delimited_int( const CTWPVec& cvec, size_t fromPos, size_t& endPos, BarzerNumber& theInt )
{
	{ // first token
	const CToken& ctok1 = cvec[fromPos].first;
	if( !ctok1.isNumber() || !ctok_number_can_be_in_commadelim(true,ctok1) || fromPos+2>= cvec.size() )
		return false;
	} // end of first process
	size_t lastCommaEnd = cvec.size()-1;
	size_t pos = fromPos+1;
	size_t numZeroBlocks = 0;
	for( ; pos < lastCommaEnd && numZeroBlocks< 6; ) {
		if( !cvec[pos].first.isPunct(',') || !ctok_number_can_be_in_commadelim(false,cvec[pos+1].first) ) {
			break;
		} else{
			++numZeroBlocks;
			pos+= 2;
		}
	}
	if( !numZeroBlocks )
		return false;
	endPos = pos;
	int64_t intResult = 0;
	for( size_t i = fromPos; i< endPos; i+=2, --numZeroBlocks ) {
		const BarzerNumber& num = cvec[i].first.getNumber();
		intResult += num.getInt() * get_decimal_factor( numZeroBlocks );
	}
	theInt.set( intResult );
	return true;
}

} // end of anon namespace

int QLexParser::advancedNumberClassify( Barz& barz, const QuestionParm& qparm )
{
    CTWPVec& cvec = barz.getCtVec();
    // TTWPVec& tvec = barz.getTtVec();

	//// this only processes positive
	BarzerNumber theInt;
	size_t endPos= 0;
	for( size_t i =0; i< cvec.size(); ++i ) {
		CToken& t = cvec[i].first;
		if( is_comma_delimited_int(cvec, i,endPos,theInt ) ) {
			t.setNumber(theInt);
			for( size_t j = i+1; j< endPos; ++j ) {
				CToken& clearTok = cvec[j].first;
				t.qtVec.insert( t.qtVec.end(), clearTok.qtVec.begin(),clearTok.qtVec.end() );
				clearTok.clear();
			}
			// *****
			continue;
		}
		if( t.isNumber() ) {
			size_t i_dot = i+1, i_frac = i+2;
			if( i_frac < cvec.size() ) {
				CToken& dotTok = cvec[i_dot].first;
				CToken& fracTok = cvec[i_frac].first;
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

int QLexParser::advancedBasicClassify( Barz& barz, const QuestionParm& qparm )
{
    CTWPVec& cvec = barz.getCtVec();
    // TTWPVec& tvec = barz.getTtVec();

	// transforms
	advancedNumberClassify(barz, qparm);
	removeBlankCTokens( cvec );
    /// reclassifying punctuation if needed
    for( CTWPVec::iterator i = cvec.begin(); i!= cvec.end(); ++i ) {
        CToken& ctok = i->first;
        char punct =  ctok.getPunct();
        if( punct ) {
            char theString[] = { punct, 0 };
		    const StoredToken* storedTok = dtaIdx->getStoredToken( theString );
            if( storedTok ) {
                ctok.storedTok = storedTok;
                ctok.syncClassInfoFromSavedTok();
            }
        }
    }
	return 0;
}

namespace {
bool isStringAscii( const std::string& s )
{
	for( std::string::const_iterator i= s.begin(); i!= s.end(); ++i ) {
		if( !isascii(*i) ) return false;
	}
	return true;
}

} // anonymous namespace ends

struct SpellCorrectResult
{
	int d_result;
	size_t d_nextTtok, d_nextCtok;

	SpellCorrectResult () : d_result(0) { }

	SpellCorrectResult (int res, size_t ct, size_t tt) : 
        d_result (res) , d_nextTtok (tt) , d_nextCtok (ct) {}
};

SpellCorrectResult QLexParser::trySpellCorrectAndClassify (PosedVec<CTWPVec> cPosVec, PosedVec<TTWPVec> tPosVec, const QuestionParm& qparm)
{
	CToken& ctok = cPosVec.element().first;
	TToken& ttok = tPosVec.element().first;

	const char* t = ttok.buf;
	size_t t_len = ttok.len;
	if( t_len > BZSpell::MAX_WORD_LEN )
		return SpellCorrectResult (0, ++cPosVec, ++tPosVec);
	const GlobalPools& gp = d_universe.getGlobalPools();

	bool isAsciiToken = ttok.isAscii();
	std::string ascifiedT;
	// bool startWithSpelling = true;
	const BZSpell* bzSpell = d_universe.getBZSpell();

	const char* correctedStr = 0;
	/// there are non ascii characters in the string
	const char* theString = t;

	if( !isAsciiToken ) {
		if( ay::umlautsToAscii( ascifiedT, t ) )
			correctedStr = ascifiedT.c_str();
		theString = ( correctedStr? correctedStr: ascifiedT.c_str() ) ;
	}
	uint32_t strId = 0xffffffff;
	int isUsersWord =  bzSpell->isUsersWord( strId, theString ) ;


	char buf[ BZSpell::MAX_WORD_LEN ] ;
	// trying lower case
	if( !isUsersWord ) {
	    if( gp.isWordInDictionary(strId) ) {
			std::string stemmedStr;
			strId = bzSpell->getStemCorrection( stemmedStr, theString );
            if( strId == 0xffffffff && qparm.isStemMode_Aggressive() )
			    strId = bzSpell->getAggressiveStem( stemmedStr, theString );

			if( strId != 0xffffffff ) {
				correctedStr = gp.string_resolve( strId ) ;
                if( correctedStr ) {
                    isUsersWord =  bzSpell->isUsersWord( strId, correctedStr ) ;
                    if( isUsersWord )
                        theString = correctedStr;
                }
			} else {
		        ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
		        return SpellCorrectResult (0, ++cPosVec, ++tPosVec);
            }
	    } else {
		    strncpy( buf, theString, BZSpell::MAX_WORD_LEN );
		    buf[ sizeof(buf)-1 ] = 0;
            size_t buf_len=strlen(buf);
            int16_t lang = Lang::getLang( buf, buf_len );
            if( lang == LANG_ENGLISH ) {
		        bool hasUpperCase = false;
		        for( char* ss = buf; *ss; ++ss ) {
			        if( isupper(*ss) ) {
				        if( !hasUpperCase ) hasUpperCase= true;
				        *ss = tolower(*ss);
			        }
		        }
		        theString = buf;
		        if( hasUpperCase ) {
			        isUsersWord =  bzSpell->isUsersWord( strId, theString ) ;
		        }

		        if( !isUsersWord && isupper(theString[0]) ) {
			        ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
			        return SpellCorrectResult (0, ++cPosVec, ++tPosVec);
		        }
            } else if(Lang::isTwoByteLang(lang)) {
                bool hasUpperCase = Lang::convertTwoByteToLower( buf, buf_len, lang );
                if( hasUpperCase )
                    isUsersWord =  bzSpell->isUsersWord( strId, buf ) ;
                theString = buf;
            }
        }
	}

	if( !isUsersWord ) {
		strId = bzSpell->getSpellCorrection( theString );
		if( strId != 0xffffffff ) {
			 correctedStr = gp.string_resolve( strId ) ;
		} else { // trying stemming
			std::string stemmedStr;
			strId = bzSpell->getStemCorrection( stemmedStr, theString );
			if( strId != 0xffffffff ) {
				correctedStr = gp.string_resolve( strId ) ;
			} else if( (stemmedStr.length() > MIN_SPELL_CORRECT_LEN) && strlen(theString) != stemmedStr.length() ) {
                // spelling correction failed but the string got stemmed
                strId = bzSpell->getSpellCorrection( stemmedStr.c_str() );
                if( strId != 0xffffffff )
                    correctedStr = gp.string_resolve( strId ) ;
            }
		}

		bool lastFailed = false;
		if( strId == 0xffffffff ) {
			std::string stemmedStr;
            if( qparm.isStemMode_Aggressive() ) {
			    strId = bzSpell->getAggressiveStem( stemmedStr, theString );
                if(strId == 0xffffffff) {
                    lastFailed = true;
                } else {
                    correctedStr = gp.string_resolve( strId ) ;
                    theString= correctedStr;
                }
            } else  {
                lastFailed = true;
            }
		} else
			theString= correctedStr;

        //// attempting split correction - 
        /// trying to split the word into two 
		const int MultiwordLen = BZSpell::MAX_WORD_LEN / 4;
		if (strId == 0xffffffff && t_len < MultiwordLen)
		{
			char dirty [BZSpell::MAX_WORD_LEN];
            int16_t lang = Lang::getLang (theString, t_len);
			const size_t step = Lang::isTwoByteLang (lang) ? 2 : 1;

			for (size_t i = step; i < t_len - step; i += step)
			{
				std::memcpy (dirty, theString, i);
				dirty [i] = 0;
		        const StoredToken* tmpTok = dtaIdx->getStoredToken( dirty );
                uint32_t left = 0xffffffff;
                if( !tmpTok || tmpTok->isStemmed() ) { // if no token found or token is pure stem
                    if( i < MIN_SPELL_CORRECT_LEN*step ) 
                        continue;
                    else
                        left = bzSpell->getSpellCorrection (dirty,false) ;
                } else 
                    left = tmpTok->getStringId() ;
                
				if (left == 0xffffffff)
					continue;

                const char* rightDirty = theString + i;

		        tmpTok = dtaIdx->getStoredToken( rightDirty );
				uint32_t right = 0xffffffff;
                if( !tmpTok ) {
                    if( t_len - step- i < MIN_SPELL_CORRECT_LEN*step ) 
                        continue;
                    else 
                        right = bzSpell->getSpellCorrection (rightDirty,false) ;
                } else
                    right = tmpTok->getStringId() ;

				if (right == 0xffffffff)
					continue;

				const char *leftCorr = gp.string_resolve (left);
				const char *rightCorr = gp.string_resolve (right);

				CToken newTok = ctok;
				// newTok.addSpellingCorrection (t, rightCorr);
				const StoredToken *st = dtaIdx->getStoredToken (rightCorr);
				if (st)
				{
					newTok.storedTok = st;
					newTok.syncClassInfoFromSavedTok ();
				}
				else
					newTok.setClass (CTokenClassInfo::CLASS_MYSTERY_WORD);

                std::string reportedCorrection(leftCorr);
                reportedCorrection.reserve( 4+strlen(rightCorr) );
                reportedCorrection.push_back( ' ' ); 
                reportedCorrection += rightCorr;

				ctok.addSpellingCorrection (t, reportedCorrection.c_str());
				st = dtaIdx->getStoredToken (leftCorr);
				if (st)
				{
					ctok.storedTok = st;
					ctok.syncClassInfoFromSavedTok ();
				}
				else
					ctok.setClass (CTokenClassInfo::CLASS_MYSTERY_WORD);

                ++cPosVec;

				cPosVec.vec().insert (cPosVec.posIterator(), std::make_pair (newTok, cPosVec.pos() ));
				return SpellCorrectResult (1, ++cPosVec, ++tPosVec);
			}
		}
        //// end of split correction attempt 

		if (lastFailed)
		{
			correctedStr = 0;
			theString = 0;
		}
	}
	if( theString ) {
		const StoredToken* storedTok = dtaIdx->getStoredToken( theString );

		if( storedTok ) {
			ctok.storedTok = storedTok;
			ctok.syncClassInfoFromSavedTok();

		} else {
			ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
		}
		if( correctedStr )
			ctok.addSpellingCorrection( t, correctedStr );
		return SpellCorrectResult (1, ++cPosVec, ++tPosVec);
	} else
		ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
	return SpellCorrectResult (-1, ++cPosVec, ++tPosVec);
}

int QLexParser::singleTokenClassify( Barz& barz, const QuestionParm& qparm )
{
    CTWPVec& cVec = barz.getCtVec();
    TTWPVec& tVec = barz.getTtVec();

	cVec.resize( tVec.size() );
	if( !dtaIdx )
		return ( err.e = QLPERR_NULL_IDX, 0 );

	const BZSpell* bzSpell = d_universe.getBZSpell();
	bool isQuoted = false;
    size_t cPos = 0, tPos = 0;
	while (cPos < cVec.size() && tPos < tVec.size()) {
		TToken& ttok = tVec[tPos].first;
		const char* t = ttok.buf;
		// cPos->second = std::distance (cVec.begin (), cPos);
		CToken& ctok = cVec[cPos].first;
		ctok.setTToken (ttok, cPos );
		bool wasStemmed = false;

		++cPos;
		++tPos;
		if( !t || !*t || isspace(*t)  ) { // this should never happen
			ctok.setClass( CTokenClassInfo::CLASS_SPACE );
			continue;
		} else if( (*t) == '.' ) {
            ctok.setClass( CTokenClassInfo::CLASS_PUNCTUATION );
            if( *t == '"' ) isQuoted = !isQuoted;
        } else if( qparm.isAutoc || !tryClassify_integer(ctok,t) ) {
			uint32_t usersWordStrId = 0xffffffff;
			const StoredToken* storedTok = ( bzSpell->isUsersWord( usersWordStrId, t ) ?
				dtaIdx->getStoredToken( t ): 0 );

            bool isNumber = ( !qparm.isAutoc && tryClassify_number(ctok,t)); // this should probably always be false
			if( storedTok ) { ///
				///
				ctok.storedTok = storedTok;
				ctok.syncClassInfoFromSavedTok();
			} else { /// token NOT matched in the data set
                if( ispunct(*t)) {
                    ctok.setClass( CTokenClassInfo::CLASS_PUNCTUATION );
                    if( *t == '"' ) isQuoted = !isQuoted;
                } else if( !isNumber ) {
					/// fall thru - this is an unmatched word

					if( !isQuoted /*d_universe.stemByDefault()*/ )
					{
						SpellCorrectResult scr = trySpellCorrectAndClassify (
                            PosedVec<CTWPVec> (cVec, cPos-1),
                            PosedVec<TTWPVec> (tVec, tPos-1), 
                            qparm
                        );
                        if( scr.d_nextCtok < cVec.size() ) 
						    cPos = scr.d_nextCtok;
                        if( scr.d_nextTtok < tVec.size() )
						    tPos = scr.d_nextTtok;
						if (scr.d_result > 0)
							wasStemmed = true;
	 				}
	 				else
					{
						ctok.setClass( CTokenClassInfo::CLASS_MYSTERY_WORD );
					}
				}
			}
		    /// stemming
		    if( !isNumber && bzSpell && ctok.isWord() && d_universe.stemByDefault() && !wasStemmed ) {
			    std::string strToStem( ttok.buf, ttok.len );
			    std::string stem;
			    if( bzSpell->stem( stem, strToStem.c_str() ) )
				    ctok.stemTok = dtaIdx->getStoredToken( stem.c_str() );
		    }
		    if( ctok.storedTok && ctok.stemTok == ctok.storedTok )
			    ctok.stemTok = 0;
		}
	}
	if( cVec.size() > MAX_CTOKENS_PER_QUERY ) {
		cVec.resize( MAX_CTOKENS_PER_QUERY );
	}

	return 0;
}

int QLexParser::lex( Barz& barz, const QuestionParm& qparm )
{
    CTWPVec& cVec = barz.getCtVec();
    TTWPVec& tVec = barz.getTtVec();

	err.clear();
	cVec.clear();
	/// convert every ttoken into a single ctoken
	singleTokenClassify( barz, qparm );
	/// try grouping tokens and matching basic compounded tokens
	/// non language specific
	advancedBasicClassify( barz, qparm );

	/// perform advanced language specific lexing
	langLexer.lex( cVec, tVec, qparm );

	if( cVec.size() > MAX_CTOKENS_PER_QUERY ) {
		cVec.resize( MAX_CTOKENS_PER_QUERY );
		AYLOG(ERROR) << "query truncated\n";
	}
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
	enum {
		CHAR_UNKNOWN=0,
		CHAR_ALPHA,
		CHAR_DIGIT,
		CHAR_UTF8
	};
	int prevChar = CHAR_UNKNOWN;
#define PREVCHAR_NOT( t ) ( prevChar && (CHAR_##t != prevChar) )


	size_t q_len = strlen(q);
	if( q_len > MAX_QUERY_LEN ) {
		q_len = MAX_QUERY_LEN;
	}
	const char* s_end = q+ q_len;
	const char* s = q;
	for( ; s!= s_end; ++s ) {

		char c = *s;
		if( !isascii(c) ) {
			if( !ay::is_diacritic(s) && PREVCHAR_NOT(UTF8) ) {
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
			if( isdigit(c) ) {
				prevChar = CHAR_DIGIT;
			} else {
				prevChar =CHAR_ALPHA;
			}
		} else
			prevChar = CHAR_ALPHA;
	    lc=c;
	}
	if( tok ) {
		ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
		tok = 0;
	}

	if( ttwp.size()  > MAX_NUM_TOKENS ) {
		ttwp.resize(MAX_NUM_TOKENS);
	}

	return ( ttwp.size() - origSize );
#undef PREVCHAR_NOT
}

} // barzer namespace
