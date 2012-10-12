#include <zurch_tokenizer.h>
#include <ay/ay_translit_ru.h>

namespace zurch {
/*
void ZurchWordNormalizer::normalize( ZurchTokenVec& dest, const ZurchTokenVec& src, NormalizerEnvironment& env ) const
{
    dest.clear();
    dest.reserve( src.size() );

    for( ZurchTokenVec::const_iterator i = src.begin(); i!= src.end(); ++i ) 
    {
        if( i->str.length() > 1 ) {
            dest.resize( dest.size() +1 );
            ZurchToken& t = dest.back();
            t=*i;
            if( d_bzspell->stem( env.stem, i->str.c_str() ) ) {
                dest.str = env.stem;
            } else {
                
            }
        }
    }
}
*/
void ZurchWordNormalizer::normalize( std::string& dest, const char* src, NormalizerEnvironment& env ) const
{
    env.stem.clear();
    if( !d_bzspell->stem( env.stem, src ) )
        env.stem=src;
    env.translit.clear();
    ay::tl::en2ru(env.stem.c_str(), env.stem.length(), env.translit);
    env.dedupe.clear();
    ay::tl::dedupeRussianConsonants(env.dedupe, env.translit.c_str(), env.translit.size() );
    if( env.bastardizer ) {
        env.bastardizer->transform( env.dedupe.c_str(), env.dedupe.length(), env.bastardized );
    }
    
    dest = env.bastardized;
}

void ZurchTokenizer::config( const boost::property_tree::ptree& pt )
{
#warning implement ZurchTokenizer::config
/// we'll have configurable separators
}

namespace {

struct ZurchTokenizer_CB {
    ZurchTokenVec& vec;
    
    inline bool operator()( const ay::parse_token& tok ) 
    {
        vec.push_back( ZurchToken(tok) );
        vec.back().str.assign( tok.getBuf(), tok.getBuf_sz() );
        return true;
    }
    
    ZurchTokenizer_CB( ZurchTokenVec& v ) : vec(v) {}
};

}// anonymous namespace 

void ZurchTokenizer::tokenize( ZurchTokenVec& vec, const char* str, size_t str_sz )
{
    ZurchTokenizer_CB cb(vec);
    d_tokenizer.tokenize( str, str_sz, cb );
}

} // namespace zurch
