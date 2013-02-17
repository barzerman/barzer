
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
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
    static bool stem( std::string& out, const char* s ) ;
};

} // namespace barzer
