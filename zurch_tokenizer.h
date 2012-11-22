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

/// optimized for tokenizing large documents 
/// there are two ways of specifying separators:
/// heuristicBit.set - see the flags in ay_tokenizer - this is preferred 
/// for potential tokenizers it's d_tokenizer.heuristicPotentialBit 
/// in addition to heuristics a list of arbitrary separators can be added 
class ZurchTokenizer {
    enum { 
        ZTF_DETECTID, /// apply id detection heuristic (numbers+dots+...)
        ZTF_EMIT_SPACES, /// when on "a b" becomes "a" " " "b" otherwise "a" "b"
        ZTF_MAX 
    };
    ay::bitflags< ZTF_MAX > d_bit;
    ay::callback_tokenizer d_tokenizer;
public:
    ay::callback_tokenizer& tokenizer() { return d_tokenizer; }
    const ay::callback_tokenizer& tokenizer() const { return d_tokenizer; }

    ay::bitflags< ZTF_MAX >&       bits()       { return d_bit; }
    const ay::bitflags< ZTF_MAX >& bits() const { return d_bit; }
    bool emit_spaces() const { return d_bit.checkBit(ZTF_EMIT_SPACES); }
    
    ZurchTokenizer( const char* sep=0 ) {
        if( sep ) 
            d_tokenizer.addSeparators(sep);
        else {
            d_tokenizer.heuristicBit.set( ay::callback_tokenizer::TOK_HEURBIT_ASCII_SPACE );
            d_tokenizer.heuristicBit.set( ay::callback_tokenizer::TOK_HEURBIT_ASCII_ISPUNCT );
            d_tokenizer.heuristicPotentialBit.set( ay::callback_tokenizer::TOK_HEURBIT_ASCII_NONALNUM );
        }
    }
    void addSeparators( const char* s ) { d_tokenizer.addSeparators(s); }

    void clearHeuristics() { d_tokenizer.heuristicBit.clear(); }
    void clearPotentialHeuristics() { d_tokenizer.heuristicPotentialBit.clear(); }
    void clearExtraSeparators() { d_tokenizer.clearSeparators(); }

    void tokenize( ZurchTokenVec&, const char* str, size_t str_sz );
    void config( const boost::property_tree::ptree& );
};

class ZurchWordNormalizer {
    const barzer::BZSpell* d_bzspell;
public:
    enum {
        STRAT_RU_TRANSLIT_CONSONANT, 
        STRAT_RU_TRANSLIT_VOWEL
    };
    /// MUST be single threaded  . this object is needed so that intermediate heap objects dont get reallocated all the time 
    struct NormalizerEnvironment {
        std::string stem, translit, dedupe, bastardized;
        int  strategy;
        const barzer::HashingSpellHeuristic* bastardizer;

        NormalizerEnvironment( int st = STRAT_RU_TRANSLIT_CONSONANT ) : strategy(st), bastardizer(0) {}
        NormalizerEnvironment( const barzer::HashingSpellHeuristic* b, int st = STRAT_RU_TRANSLIT_CONSONANT ) : strategy(st), bastardizer(b) {}
        void clear()
        {
            stem.clear();
            translit.clear();
            dedupe.clear();
            bastardized.clear();
        }
    };

    ZurchWordNormalizer( const barzer::BZSpell* spell=0 ) : d_bzspell(spell) {}
    void setBZSpell( const barzer::BZSpell* spell ) { d_bzspell = spell; }
    const char* normalize( std::string& dest, const char* src, NormalizerEnvironment& env ) const;
};

} // namespace zurch
