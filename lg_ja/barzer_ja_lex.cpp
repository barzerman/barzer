
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <lg_ja/barzer_ja_lex.h>
#include <barzer_bzspell.h>
#include <ay_util_time.h>

namespace barzer {
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

int LangModelJa::lex(Barz& b, QTokenizer &t, QLexParser &qp, const QuestionParm& qparm) const {
	return 0;

}
}
