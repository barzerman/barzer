#ifndef BARZER_RU_LEX_H 
#define BARZER_RU_LEX_H 
#include <barzer_language.h>
namespace barzer {

class QSingleLangLexer_RU : public QSingleLangLexer {
public:
QSingleLangLexer_RU() : QSingleLangLexer ( LANG_RUSSIAN ) {}
int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );

};

class BZSpell;
/// russian language stemmer 
struct Russian_Stemmer {
    static uint32_t getStemCorrection( std::string& out, const BZSpell& bzspell, const char* s, size_t s_len );
    static bool stem( std::string& out, const char* s ) const;
};

}
#endif // BARZER_RU_LEX_H 
