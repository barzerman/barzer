
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <lg_ja/barzer_ja_lex.h>
#include <barzer_bzspell.h>
#include <barzer_lexer.h>
#include <barzer_universe.h>
#include <ay_util_time.h>
#include <ay_unicode_category.h>

#include <kytea/kytea-struct.h>
#include <kytea/string-util.h>


namespace barzer {
using namespace kytea;
int QSingleLangLexer_JA::lex( CTWPVec& , const TTWPVec&, const QuestionParm& )
{
	return 0;
}

void LangModelJa::loadData(const char *path) {
	ay::stopwatch t;
	kytea.readModel(path);
	std::cerr << "Loaded kytea model from `" << path
			  << "' in " << t.calcTime() << "sec" << std::endl;

}

/*
int LangModelJa::tokenize(Barz& b, QTokenizer &t, const QuestionParm& qparm) const {
	return 0;
} //*/
namespace {
	KyteaSentence tokenize(const char *query, Kytea &kytea, TTWPVec& tVec) {
		    // Get the string utility class. This allows you to convert from
		    //  the appropriate string encoding to Kytea's internal format
		    StringUtil* util = kytea.getStringUtil();

		    // Get the configuration class, this allows you to read or set the
		    //  configuration for the analysis
		    const KyteaConfig* config = kytea.getConfig();

		    // Map a plain text string to a KyteaString, and create a sentence object

		    KyteaString surface_string = util->mapString(query); // why isnt' this fucking const
		    KyteaSentence sentence(surface_string, util->normalize(surface_string));

		    // Find the word boundaries
		    kytea.calculateWS(sentence);
		    // Find the pronunciations for each tag level
		    for(int i = 0; i < config->getNumTags(); i++)
		        kytea.calculateTags(sentence,i);

		    const KyteaSentence::Words & words =  sentence.words;
		    std::cout << "found " << words.size() << " words" << std::endl;
		    size_t byte_offset = 0, char_offset = 0, byte_len = 0, char_len = 0;
		    for(int i = 0; i < (int)words.size(); i++) {
		    	auto &surf = words[i].surface;
		    	std::string word = util->showString(surf);
		    	char_len = surf.length();
		    	byte_len = word.size();

				tVec.push_back( TTWPVec::value_type(
				            TToken(
				            	query+byte_offset,
				                byte_len,
				                char_offset,
				                char_len,
				                byte_offset
				            ),
				            tVec.size()
				        ));
				byte_offset += byte_len;
				char_offset += char_len;
		    }
        return sentence;
	}
	bool tryNumber(Barz &barz, CToken& ctok, const TToken& ttok, ay::StrUTF8 &ut){
		/*const char* beg = ttok.buf.c_str();
		const char* end = ttok.buf.c_str()+ttok.buf.length();
		*/
		size_t len = ut.length();
	    for( size_t i = 0; i < len; ++i ) {
	    	uint32_t char32 = ut.getGlyph(i).toUTF32();
	    	if (!ay::UnicodeClassifier::isNumber(char32)) {
	    		return false;
	    	}
	    }
	    if( !barz.getUniverse()->checkBit(UBIT_NC_LEADING_ZERO_ISNUMBER) &&
	    		len > 1 &&
	    		ut.getGlyphStart(0)[0] == '0') {
	    	return false;
	    }
		ctok.setClass( CTokenClassInfo::CLASS_NUMBER );
		// need to patch it to recognize other unicode digits beside '0'..'9'
		ctok.number().setInt( ttok.buf.c_str() );
		ctok.number().setAsciiLen( ttok.buf.length() );
		return true;
	}
}
int LangModelJa::lex(Barz& barz, QTokenizer &t, QLexParser &qp, const QuestionParm& qparm) const {

    CTWPVec& cVec = barz.getCtVec();
    TTWPVec& tVec = barz.getTtVec();

    qp.err.clear();
    tVec.clear();
    cVec.clear();

    const char *query = barz.getOrigQuestion().data();
    StringUtil* util = kytea.getStringUtil();
    KyteaSentence sentence = tokenize(query, kytea, tVec);
    auto &words = sentence.words;

    cVec.resize( tVec.size() );

    const BZSpell *bzSpell = barz.getUniverse()->getBZSpell();
    //if (!bzSpell) std::cout << "NO BZSPELL\n";

    //bool isQuoted = false;
    bool shouldStem = !qparm.isAutoc;
    size_t cPos = 0, tPos = 0;
	while (cPos < cVec.size() && tPos < tVec.size()) {
		size_t wpos = tPos;
		TToken& ttok = tVec[tPos].first;
		CToken& ctok = cVec[cPos].first;
        size_t ctokCpos = cPos;
		ctok.setTToken (ttok, cPos );
		++tPos;
		++cPos;

		const char* t = ttok.buf.c_str();
		if (!t || !*t) {
			ctok.setClass( CTokenClassInfo::CLASS_SPACE );
			continue;
		}
		ay::StrUTF8 ut(t, ttok.buf.length());
		uint32_t char32 = ut.getGlyph(0).toUTF32();

		const StoredUniverse *uni = barz.getUniverse();
		const DtaIndex &dtaIdx = uni->getDtaIdx();

		if (tryNumber(barz, ctok, ttok, ut)) {
		    uint32_t usersWordStrId = 0xffffffff;
            const StoredToken* storedTok = ( bzSpell->isUsersWord( usersWordStrId, t ) ?
                dtaIdx.getStoredToken( t ): 0 );
            if( storedTok )
                ctok.number().setStringId( storedTok->getStringId() );
			continue;
		} else if (ut.length() > 1) {
			// space/punct can only be a single glyph
			// or at least for now
		} else if (ay::UnicodeClassifier::isSpace(char32)) {
			ctok.setClass( CTokenClassInfo::CLASS_SPACE );
			continue;
		} else if (ay::UnicodeClassifier::isPunct(char32)) {
			ctok.setClass(CTokenClassInfo::CLASS_PUNCTUATION );
			continue;
		}

	    uint32_t usersWordStrId = 0xffffffff;
        const StoredToken* storedTok = ( bzSpell->isUsersWord( usersWordStrId, t ) ?
            dtaIdx.getStoredToken( t ): 0 );

		bool isUsersWord = bzSpell->isUsersWord( usersWordStrId, t ) ;
		if (isUsersWord) {
			ctok.storedTok = storedTok;
			ctok.syncClassInfoFromSavedTok();
			ctok.setClass(CTokenClassInfo::CLASS_WORD);
		} else {
			ctok.setClass(CTokenClassInfo::CLASS_MYSTERY_WORD);
		}

		// use word's reading as a stem.
		// probably the best we can do
		auto &tags = words[wpos].tags;
		if ((shouldStem || (uni->stemByDefault() && !ctok.getStemTok())) && tags.size() > 0) {
			auto &stags = tags[tags.size()-1];
			if (stags.size() > 0) {
				const char *reading = util->showString(stags[0].first).data();
				const StoredToken* rtok = dtaIdx.getStoredToken( reading );
				if (rtok) {
					const char *m = dtaIdx.getStrPool()->printableStr(rtok->stringId);
					std::cout << "FOUND stem for " << t << ": " << m << std::endl;
					ctok.setStemTok( rtok );
					ctok.stem.assign(m);
				}
			}
		}

		ctok.syncStemAndStoredTok(*uni);
		std::cout << ctok.storedTok << ":" << ctok.stemTok << std::endl;
	}

    qp.separatorNumberGuess( barz, qparm );
	/// try grouping tokens and matching basic compounded tokens
	/// non language specific
	qp.advancedBasicClassify( barz, qparm );

    if( cVec.size() > qp.d_maxCtokensPerQuery ) {
        cVec.resize( qp.d_maxCtokensPerQuery );
        AYLOG(ERROR) << "query truncated\n";
        barz.barzelTrace.setErrBit( BarzelTrace::ERRBIT_TRUNCATED_TOKENS );
    }
    return 0;


}
}
