#include <zurch_tokenizer.h>
#include <ay/ay_translit_ru.h>

namespace zurch {
const char* ZurchWordNormalizer::normalize( std::string& dest, const char* src, NormalizerEnvironment& env ) const
{
    env.stem.clear();
    if( d_bzspell ) {
        if( !d_bzspell->stem( env.stem, src ) )
            env.stem=src;
    } else {
        env.stem=src;
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
