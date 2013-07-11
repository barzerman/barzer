
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
class Russian_StemmerSettings;

/// russian language stemmer 
struct Russian_Stemmer {
    static uint32_t getStemCorrection( std::string& out, const BZSpell& bzspell, const char* s, size_t s_len );
    static bool stem( std::string& out, const char* s, const Russian_StemmerSettings* settings ) ;
};

//// stemmer settings 
class Russian_StemmerSettings {
public:
    enum {
        BIT_2CHAR_VERB_SUFFIX, // by default they're ignored

        // add new bits above this line
        BIT_MAX
    };
    ay::bitflags< BIT_MAX > d_bit;
};
} // namespace barzer
