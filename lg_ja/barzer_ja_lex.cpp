
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

int LangModelJa::lex(Barz& barz, QTokenizer &t, QLexParser &qp, const QuestionParm& qparm) const {

    CTWPVec& cVec = barz.getCtVec();
    TTWPVec& tVec = barz.getTtVec();

    qp.err.clear();
    cVec.clear();
    /// convert every ttoken into a single ctoken

    // Get the string utility class. This allows you to convert from
    //  the appropriate string encoding to Kytea's internal format
    StringUtil* util = kytea.getStringUtil();

    // Get the configuration class, this allows you to read or set the
    //  configuration for the analysis
    const KyteaConfig* config = kytea.getConfig();

    // Map a plain text string to a KyteaString, and create a sentence object
    KyteaString surface_string = util->mapString("これはテストです。"); // why isnt' this fucking const
    KyteaSentence sentence(surface_string, util->normalize(surface_string));

    // Find the word boundaries
    kytea.calculateWS(sentence);
    // Find the pronunciations for each tag level
    for(int i = 0; i < config->getNumTags(); i++)
        kytea.calculateTags(sentence,i);




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

	return 0;

}
}
