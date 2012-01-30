#include <lg_ru/barzer_ru_lex.h>
#include <barzer_bzspell.h>
#include <barzer_ru_stemmer.h>

namespace barzer {
int QSingleLangLexer_RU::lex( CTWPVec& , const TTWPVec&, const QuestionParm& )
{
	return 0;
}

///  russian morphology (stemmer) 
// here we already assume that the string has an even number of chars (2 per each russian letter)
uint32_t Russian_Stemmer::getStemCorrection( std::string& out, const BZSpell& bzspell, const char* s, size_t s_len )
{
    if( s_len < 1 || s_len>= BZSpell::MAX_WORD_LEN ) 
        return 0xffffffff;

    return 0xffffffff;
}

bool Russian_Stemmer::stem( std::string& out, const char* s ) 
{
    return Russian::normalize(out,s);
}

}
