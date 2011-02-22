#ifndef BARZER_EN_LEX_H 
#define BARZER_EN_LEX_H 
#include <barzer_language.h>

namespace barzer {

class QSingleLangLexer_EN : public QSingleLangLexer {
public:
QSingleLangLexer_EN() : QSingleLangLexer ( LANG_ENGLISH ) {}
int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );

};


}
#endif // BARZER_EN_LEX_H 
