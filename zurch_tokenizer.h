#pragma once

#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <barzer_parse_types.h>
#include <barzer_parse_types.h>
#include <boost/property_tree/ptree.hpp>
#include <ay/ay_tokenizer.h>
#include <ay/ay_bitflags.h>

namespace zurch {

struct ZurchToken {
    ay::parse_token tok;
    std::string     str;

    ZurchToken( const ay::parse_token& t ) : tok(t) {}
};

inline std::ostream& operator<<( std::ostream& fp, const ZurchToken& zt )
{
    return( fp << "\"" << zt.str << "\"(" << zt.tok << ")" );
}
typedef std::vector<ZurchToken> ZurchTokenVec;

#define ZURCH_DEFAULT_SEPARATORS ",./ \n-;'`|"
/// optimized for tokenizing large documents 
class ZurchTokenizer {
    enum { ZTF_DETECTID, ZTF_MAX };
    ay::bitflags< ZTF_MAX > d_bit;
    ay::callback_tokenizer d_tokenizer;
public:
    ay::callback_tokenizer& tokenizer() { return d_tokenizer; }
    const ay::callback_tokenizer& tokenizer() const { return d_tokenizer; }

    ay::bitflags< ZTF_MAX >&       bits()       { return d_bit; }
    const ay::bitflags< ZTF_MAX >& bits() const { return d_bit; }
    
    ZurchTokenizer( const char* sep=ZURCH_DEFAULT_SEPARATORS ) {
        if( sep ) 
            d_tokenizer.addSeparators(sep);
    }

    void tokenize( ZurchTokenVec&, const char* str, size_t str_sz );
    void config( const boost::property_tree::ptree& );
};

class ZurchWordNormalizer {
    barzer::BZSpell* d_bzspell;
public:
    enum {
        STRAT_RU_TRANSLIT_CONSONANT, 
        STRAT_RU_TRANSLIT_VOWEL
    };
    /// MUST be single threaded  . this object is needed so that intermediate heap objects dont get reallocated all the time 
    struct NormalizerEnvironment {
        std::string stem, translit, dedupe;
        int  strategy;
        NormalizerEnvironment( int st = STRAT_RU_TRANSLIT_CONSONANT ) : strategy(st) {}
    };

    ZurchWordNormalizer( barzer::BZSpell* spell ) : d_bzspell(spell) {}
    void normalize( std::string& dest, const char* src, NormalizerEnvironment& env ) const;
};

} // namespace zurch
