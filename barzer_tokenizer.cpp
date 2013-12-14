
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_tokenizer.h>
#include <barzer_barz.h>
#include <barzer_universe.h>

namespace barzer {

////////////// TOKENIZE
///////////////////////////////////

int QTokenizer::tokenize_strat_space(Barz& barz, const QuestionParm& qparm )
{
    TTWPVec& ttwp = barz.getTtVec();
    ttwp.clear();

    /// moving thru all glyphs (utf8 chars) in qutf8 and breaking it up on spaces
    bool lastWasSpace = false;
    const char* prevGlyphStart = 0;
    size_t     startGlyph = 0;

    size_t questionOrigUTF8_len = barz.questionOrigUTF8.length();
    const char* questionOrigUTF8_start = ( questionOrigUTF8_len ? barz.questionOrigUTF8.getGlyphStart(0) : 0 );

    for( size_t i = 0; i< questionOrigUTF8_len; ++i ) {
        const char* s = barz.questionOrigUTF8.getGlyphStart(i);
        if( s && isspace(*s) ) { // we got the space 
            if( !lastWasSpace ) {
                if( prevGlyphStart )
                    ttwp.push_back( 
                        TTWPVec::value_type(
                            TToken(
                                prevGlyphStart,
                                (s-prevGlyphStart),
                                startGlyph,
                                (i-startGlyph),
                                (prevGlyphStart-questionOrigUTF8_start)
                            ), 
                            ttwp.size() 
                        )
                    );
                ttwp.push_back( 
                    TTWPVec::value_type(
                        TToken(s,1).setOrigOffsetAndLength((s-questionOrigUTF8_start),1), 
                        ttwp.size() 
                    )
                );
                lastWasSpace= true;
            } /// 
        } else if( (lastWasSpace || !prevGlyphStart) ) { /// this is not space but last one was space 
            prevGlyphStart= s;
            startGlyph=i;
            lastWasSpace=false;
        }   // this is not a space and last one wasnt space - nothing to do 
    }
    if( prevGlyphStart && !lastWasSpace ) 
        ttwp.push_back( TTWPVec::value_type(
            TToken(
                prevGlyphStart,
                (barz.questionOrigUTF8.getBufEnd()-prevGlyphStart),
                startGlyph, 
                (barz.questionOrigUTF8.length()-startGlyph),
                prevGlyphStart-questionOrigUTF8_start
            ),
            ttwp.size() 
        ));

    return 0;
}
int QTokenizer::tokenize( Barz& barz, const TokenizerStrategy& strat, const QuestionParm& qparm )
{
    if( strat.isDefault() ) {
        /*
        if( !d_universe.checkBit( UBIT_NO_EXTRA_NORMALIZATION ) )
            barz.extraNormalization(qparm);
        */
        return tokenize(barz.getTtVec(), barz.getOrigQuestion().c_str(), qparm );
    } else {
        ///  Barz has current question to be tokenize as well as the utf8 representation (it's up to date)
        return tokenize_strat_space(barz, qparm );
    }
}
int QTokenizer::tokenize( TTWPVec& ttwp, const char* q, const QuestionParm& qparm  )
{
	err.clear();

	size_t origSize = ttwp.size();
	char lc = 0, lc_1 = 0;
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

	if( q_len > d_maxQueryLen ) 
		q_len = d_maxQueryLen;
	
	const char* s_end = q+ q_len;
    const char* s_end_1 = s_end-1;
	const char* s = q;
	for( ; s!= s_end; ++s ) {

		char c = *s;
		if( !isascii(c) ) {
			if( CHAR_DIGIT == prevChar || (!ay::is_diacritic(s) && PREVCHAR_NOT(UTF8)) ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok).setOrigOffsetAndLength(tok-q,(s-tok)), ttwp.size() ));
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
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok).setOrigOffsetAndLength(tok-q,(s-tok)), ttwp.size() ));
					tok = 0;
				}
				ttwp.push_back( TTWPVec::value_type( TToken(s,1).setOrigOffsetAndLength(s-q,1), ttwp.size() ));
			}
			prevChar = CHAR_UNKNOWN;
		} else if( ispunct(c) ) {
			if( tok ) {
				ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok).setOrigOffsetAndLength(tok-q,s-tok), ttwp.size() ));
				tok = 0;
			}
			ttwp.push_back( TTWPVec::value_type( TToken(s,1).setOrigOffsetAndLength(s-q,1), ttwp.size() ));
			prevChar = CHAR_UNKNOWN;
		} else if( isalnum(c) ) {
			if( (isalpha(c) && (PREVCHAR_NOT(ALPHA)&&PREVCHAR_NOT(UTF8)) ) ||
				(isdigit(c) && PREVCHAR_NOT(DIGIT) ) ||
                (AY_MAYBE_RUSSIAN_CHAR(lc_1) && !isascii(lc) )
			) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok).setOrigOffsetAndLength(tok-q,s-tok), ttwp.size() ));
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

        lc_1=lc;
	    lc=c;
	}
	if( tok ) {
		ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok).setOrigOffsetAndLength(tok-q,s-tok), ttwp.size() ));
		tok = 0;
	}

	if( ttwp.size()  > d_maxNumTokens ) 
		ttwp.resize(d_maxNumTokens);

	return ( ttwp.size() - origSize );
#undef PREVCHAR_NOT
}
} // namespace barzer 
