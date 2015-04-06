
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <lg_ja/barzer_ja_lex.h>
#include <barzer_bzspell.h>
#include <barzer_lexer.h>
#include <ay_util_time.h>

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
    std::cout << "found " << tVec.size() << " ttokens" << std::endl;


    cVec.resize( tVec.size() );

    bool isQuoted = false;
    size_t cPos = 0, tPos = 0;
	while (cPos < cVec.size() && tPos < tVec.size()) {
		TToken& ttok = tVec[tPos].first;

		const char* t = ttok.buf.c_str();
		std::cout << "setting " << t << std::endl;
		ay::CharUTF8 uchar(t);
		uint32_t char32 = uchar.toUTF32();
		CToken& ctok = cVec[cPos].first;
        size_t ctokCpos = cPos;
		ctok.setTToken (ttok, cPos );
		ctok.setClass(CTokenClassInfo::CLASS_MYSTERY_WORD);
		++tPos;
		++cPos;
	}


    /*
    singleTokenClassify( barz, qparm );
    separatorNumberGuess( barz, qparm );
    /// try grouping tokens and matching basic compounded tokens
    /// non language specific
    advancedBasicClassify( barz, qparm );
   */
    if( cVec.size() > qp.d_maxCtokensPerQuery ) {
        cVec.resize( qp.d_maxCtokensPerQuery );
        AYLOG(ERROR) << "query truncated\n";
        barz.barzelTrace.setErrBit( BarzelTrace::ERRBIT_TRUNCATED_TOKENS );
    }
    return 0;


}
}
