/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <zurch_tokenizer.h>
#include <ay/ay_translit_ru.h>

namespace zurch {
const char* ZurchWordNormalizer::normalize( std::string& dest, const char* src, NormalizerEnvironment& env ) const
{
    env.stem.clear();
    int lang = barzer::LANG_UNKNOWN;
    if( d_bzspell ) {
        if( !d_bzspell->stem( env.stem, src, lang ) )
            env.stem=src;
    } else {
        env.stem=src;
        lang = barzer::Lang::getLangNoUniverse(env.stem.c_str(),env.stem.length());
    }
    
    enum { MIN_NORMALIZATION_LENGTH =4 };
    bool isTwoByteLang = barzer::Lang::isTwoByteLang(lang) ;
    if( 
        (lang == barzer::LANG_ENGLISH && env.stem.length() < MIN_NORMALIZATION_LENGTH) ||
        (lang == barzer::LANG_RUSSIAN && env.stem.length() < 2*MIN_NORMALIZATION_LENGTH) 
    ) { 
        return (dest=env.stem,dest.c_str());
    }
    if( lang == barzer::LANG_UNKNOWN_UTF8 ) {
        std::string ascifiedStr;
        if( ay::umlautsToAscii( ascifiedStr, env.stem.c_str()) ) {
            lang = barzer::Lang::getLangNoUniverse(ascifiedStr.c_str(),ascifiedStr.length());
        }
        if( lang == barzer::LANG_UNKNOWN_UTF8 )
            return (dest = ascifiedStr,dest.c_str());
    }
    env.translit.clear();
    ay::tl::en2ru(env.stem.c_str(), env.stem.length(), env.translit);
    env.dedupe.clear();
    ay::tl::dedupeRussianConsonants(env.dedupe, env.translit.c_str(), env.translit.size() );
    ay::tl::normalize_eng_onevowel( dest, env.dedupe.c_str(), env.dedupe.length() );

    return dest.c_str();
}

void ZurchTokenizer::config( const boost::property_tree::ptree& pt )
{
#warning implement ZurchTokenizer::config
/// we'll have configurable separators
}

namespace {

struct ZurchTokenizer_CB {
    ZurchTokenVec& vec;
    ZurchTokenizer& tokenizer;
    
    inline bool operator()( const ay::parse_token& tok ) 
    {
        if( tok.isWhitespace() && !tokenizer.emit_spaces() ) 
            return true;

        vec.push_back( ZurchToken(tok) );
        vec.back().str.assign( tok.getBuf(), tok.getBuf_sz() );
        return true;
    }
    
    ZurchTokenizer_CB( ZurchTokenVec& v, ZurchTokenizer& t ) : vec(v), tokenizer(t) {}
};

}// anonymous namespace 

void ZurchTokenizer::tokenize( ZurchTokenVec& vec, const char* str, size_t str_sz )
{
    ZurchTokenizer_CB cb(vec,*this);
    d_tokenizer.tokenize( str, str_sz, cb );
}

} // namespace zurch
