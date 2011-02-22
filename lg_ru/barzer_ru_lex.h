#ifndef BARZER_RU_LEX_H 
#define BARZER_RU_LEX_H 
#include <barzer_language.h>
namespace barzer {

class QSingleLangLexer_RU : public QSingleLangLexer {
public:
QSingleLangLexer_RU() : QSingleLangLexer ( LANG_RUSSIAN ) {}
int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );

};


}
#endif // BARZER_RU_LEX_H 
