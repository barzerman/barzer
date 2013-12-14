
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

namespace ru_spell {

#define ONE_HAS_PREFIX_DECL(n) inline bool one_has_prefix_##n( const char* x, const char* y, const char* p ) \
{\
    return ( \
        (ay::is_2bchar_prefix_##n( x, p ) && !ay::is_2bchar_prefix_##n(y,p)) ||\
        (!ay::is_2bchar_prefix_##n( x, p ) && ay::is_2bchar_prefix_##n(y,p))\
    );\
}

ONE_HAS_PREFIX_DECL(1) // one_has_prefix_1 - only one of the strings has the 1 char prefix
ONE_HAS_PREFIX_DECL(2) // one_has_prefix_2 - only one of the strings has the 2 char prefix
ONE_HAS_PREFIX_DECL(3) // one_has_prefix_3 - only one of the strings has the 3 char prefix
ONE_HAS_PREFIX_DECL(4) // one_has_prefix_4 - only one of the strings has the 4 char prefix

/// returns tue if src cannot be corrected to corr for some language specific reason
inline bool heuristic_correction_not_allowed( const char* src, const char* corr )
{
    return( one_has_prefix_2(src, corr, "не") );
}

} // namespace ru_spell

} // namespace barzer
