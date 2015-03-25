
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_language.h>
#include <kytea/kytea.h>
namespace barzer {

class QSingleLangLexer_JA : public QSingleLangLexer {
public:
QSingleLangLexer_JA() : QSingleLangLexer ( LANG_JAPANESE ) {}
int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );

};

class BZSpell;
class Japanese_StemmerSettings;

class LangModelJa : public LangModel {
	kytea::Kytea kytea;
public:
	void loadData(const char*);
	void setParam(const char*, const char*) {};
	//bool willTokenize() const { return true; }
	bool willLex() const { return true; }
	int lex(Barz&, QTokenizer &t, QLexParser&, const QuestionParm&) const;
	//int tokenize(Barz& b, QTokenizer &t, const QuestionParm& qparm) const;
};

namespace ja_spell {


} // namespace ja_spell

} // namespace barzer
