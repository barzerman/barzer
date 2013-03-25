/*
 * testay.cpp
 *
 *  Created on: Apr 12, 2011
 *      Author: polter
 */

#ifdef DONTCOMMENTALLOUT
#include <ay_logger.h>
#include <ay_utf8.h>
#include <ay_choose.h>
#include <ay_util_time.h>
#include <ay_util.h>
#include <ay_char.h>
#include <ay_xml_util.h>
#include <boost/format.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <ay_statistics.h>
#endif 
#include <ay_parse.h>
#include <fstream>
#include <sstream>
#include <boost/variant.hpp>

#ifdef DONTCOMMENTALLOUT
void testLogger() {
	AYLOGINIT(DEBUG);
	int i  = 0;
	AYLOGDEBUG(i + 2);
	AYLOG(ERROR) << "lala i have an error" << i++;
	AYLOG(DEBUG) << "lala i have a debug" << i++;
	AYLOG(WARNING) << "lala i have a warning" << i++;
	SETLOGLEVEL(WARNING);
	ay::Logger::getLogger()->setFile("shit.log");

	AYLOG(ERROR) << "lala i have an error" << i++;
	AYLOG(WARNING) << "lala i have a warning" << i++ << std::endl;
    ay::LogMsg logMsg(ay::Logger::DEBUG,__FILE__,__LINE__);
	AYLOG(DEBUG) << "lala i have a debug" << i++;
}

int testUTF8(int argc,char* argv[])
{
    std::string str;
    while(true) {
        std::cout << "enter string:";
        std::cin >> str;
        if( str == "." )
            return 0;
        ay::StrUTF8 s( str.c_str() );
        std::cerr << s << ":" << s.getGlyphCount() << std::endl;

        ay::StrUTF8 appended;
        for( size_t i = 0; i< s.getGlyphCount(); ++i ) {
            std::cerr << "glyph[" << i << "] =" << s[i] << std::endl;
            appended.push_back( s[i] );
        }
        ay::StrUTF8::const_iterator sbeg = s.begin();
        for( ay::StrUTF8::const_iterator i = s.begin(); i!= s.end(); ++i ) {
            std::cerr << "iterator[" << (i-sbeg) << "] =" << *i << std::endl;
        }
        std::cerr << "APPENDED:" << appended << " :::: " << appended.append(s) << std::endl;

        for( size_t i = 0; i< s.getGlyphCount(); ++i ) {
            ay::StrUTF8 x(s);
            x.swap( i, i+1 );
            std::cerr << i << "<-->" << i+1 << "=" << x << std::endl;
        }
        std::cerr << "******* Producing choices\n";
        size_t minSubstrLen = s.length();
        typedef ay::choose_n<ay::CharUTF8, ay::iterator_range_print_callback<ay::CharUTF8> > choose_n_utf8char_print;

        ay::iterator_range_print_callback<ay::CharUTF8> printCb( std::cerr );
        choose_n_utf8char_print chooser( printCb, minSubstrLen-1, minSubstrLen-1 );
        chooser( s.begin(), s.end() ) ;
        std::cerr << chooser.getNumChoices() << " choices produced\n";
        
        std::string str2( "helloаьс" );
        size_t xx = 0, xx_sz = 10000000;
        if( argc > 1 ) 
            xx_sz = atoi(argv[1]);

        ay::stopwatch localTimer;
        for( size_t i =0; i< xx_sz; ++i ) {
            if( i%2 ) {
                xx = ay::StrUTF8::glyphCount(str.c_str()) ;
            } else 
                xx = ay::StrUTF8::glyphCount(str2.c_str()) ;
        }
        std::cerr << std::dec << xx_sz << " iterations done in " << localTimer.calcTime() << " seconds";
        std::cerr << std::dec << "number of glyphs in \"" << str << "\" =" << ay::StrUTF8::glyphCount(str.c_str(),str.c_str()+str.length()) << std::endl;
    }
}
void testUTF8Split(int argc, char* argv[]) 
{
    std::string str;
    while(true) {
        std::cout << "enter string:";
        std::cin >> str;
        if( str == "." )
            return;
        ay::StrUTF8 s( str.c_str() );
        ay::StrUTF8 left,right;
        size_t glyphCount = s.getGlyphCount();
        size_t lastGlyph = ( glyphCount> 1 ? glyphCount-1:0 );
        for( size_t i=0; i< lastGlyph; ++i ) {
            left.assign( s, 0, i );
            std::cerr << left << "::" ;
            right.assign( s, i+1, lastGlyph );
            std::cerr << right << std::endl;
        }
    } 
}
void testUTF8Normalizer(int argc, char* argv[]) 
{
    std::string str2( "l'lаitéàlaUnñol");
    uint32_t xx_sz = ( argc >1 ? atoi(argv[1]) : 10000000 );
    while(true) {
        std::string str;
        std::cout << "enter string:";
        std::cin >> str;
        if( str == "." )
            return;
        ay::StrUTF8 ss( str.c_str() ), ss2(str2.c_str() );
        ay::stopwatch localTimer;
        uint32_t xx = 0;
        for( size_t i =0; i< xx_sz; ++i ) {
            if( i%2 ) 
                xx+=(ss.toUpper() ? 1: 2);
            else 
                xx+=(ss2.toUpper() ? 2:1 );

        }
        std::cerr << std::dec << xx_sz << " iterations done in " << localTimer.calcTime() << " seconds salt:" << xx << std::endl;

        ay::StrUTF8 s( str.c_str() );
        s.toUpper() ;
        std::cerr << "toupper:" << s << std::endl;
        s.toLower() ;
        std::cerr << "tolower:" << s << std::endl;
    }
}
int testStripDiacritics(int argc, char* argv[])
{
    while(true) {
        std::string str;
        std::cout << "enter string:";
        std::cin >> str;
        if( str == "." )
            return 0;
        std::string dest;
        ay::stripDiacrictics( dest, str.c_str() );
        std::cout << str << ":" << dest << std::endl;
    }
    return 0;
}
#include <ay_xml_util.h>
int testXMLEscape(int argc, char* argv[])
{
    ay::XMLStream shit(std::cerr);
    shit << "fuckshit" << std::endl;
    while(true) {
        std::string str;
        std::cout << "enter string:";
        std::cin >> str;
        if( str == "." )
            return 0;
        ay::XMLStream xmlStream(std::cerr);
        xmlStream << str << std::endl;
    }
}

int test_ay_strcasecmp (int argc, char* argv[])
{
    while( true ) {
        std::string l, r;
        std::cerr << "enter left: ";
        std::getline( std::cin, l );
        std::cerr << "enter right: ";
        std::getline( std::cin, r );
        
        std::cerr << "comparison: " << ay::ay_strcasecmp( l.c_str(), r.c_str() ) << std::endl;
        std::cerr << "left hash:" << ay::char_cp_hash_nocase()(l.c_str()) << std::endl;
        std::cerr << "right hash:" << ay::char_cp_hash_nocase()(r.c_str()) << std::endl;
    }
    return 0;
}
int test_ay_statistics(int argc, char* argv[])
{
    {
    ay::double_standardized_moments acc;
    acc(1);
    acc(2);
    acc(3);
    acc(4);
    acc(5);
    acc(6);
    acc(7);
    acc(8);
    acc(9);
    acc(10);
    std::cerr << 
        "suM:" << boost::accumulators::sum(acc) << std::endl <<
        "mean:" << boost::accumulators::mean(acc) << std::endl <<
        "count:" << boost::accumulators::count(acc) << std::endl <<
        "variance:" << boost::accumulators::variance(acc) << std::endl <<
        "skewness:" << boost::accumulators::skewness(acc) << std::endl <<
        "kurtosis:" << boost::accumulators::kurtosis(acc) << std::endl;
        //"variance:" << boost::accumulators::sum(acc) << std::endl <<
        //"suM:" << boost::accumulators::sum(acc) << std::endl;

    }
    {
    ay::double_standardized_moments_weighted acc;
    /*
    acc(1,boost::accumulators::weight=.6);
    acc(1,boost::accumulators::weight=.6);
    acc(2,boost::accumulators::weight=.6);
    acc(3,boost::accumulators::weight=1.6);
    */

    double x = boost::accumulators::skewness(acc);
    std::cerr << (boost::math::isnan(x) ? "SHIT NAN" : "NOT NAN") << std::endl;
    std::cerr << 
        "suM:" << boost::accumulators::sum(acc) << std::endl <<
        "mean:" << boost::accumulators::mean(acc) << std::endl <<
        "count:" << boost::accumulators::count(acc) << std::endl <<
        "variance:" << boost::accumulators::variance(acc) << std::endl <<
        "skewness:" << boost::accumulators::skewness(acc) << std::endl <<
        "kurtosis:" << boost::accumulators::kurtosis(acc) << std::endl;
    }
    {
        std::vector< ay::double_standardized_moments_weighted > acc;
        acc.resize(1);
        acc[0](1,boost::accumulators::weight=.6);
        acc[0](1,boost::accumulators::weight=.6);
        acc[0](2,boost::accumulators::weight=.6);
        acc[0](3,boost::accumulators::weight=1.6);
        std::cerr << sizeof( ay::double_standardized_moments_weighted ) << std::endl;

    }
    return 0;
}

struct xhtml_cb {
    size_t counter, maxStackDepth;
    xhtml_cb(): counter(0), maxStackDepth(0) {}
    std::string longestPath;
    void operator()( const ay::xhtml_parser_state& state, const char* s, size_t s_sz ) 
    {
        ++counter;
        if( state.d_tagStack.size()> maxStackDepth ) {
            maxStackDepth= state.d_tagStack.size();
            std::stringstream sstr;
            for( size_t i = 0; i< state.d_tagStack.size(); ++i ) {
                sstr << state.getTag(i) << "/";
            }
            longestPath = sstr.str();
        }
        if( state.d_cbReason ==ay::xhtml_parser_state::CB_TEXT ) {
            (std::cout << std::endl ).write( s, s_sz );
        }
       // ( std::cerr << "\n" << state.d_cbReason << ":" ).write( s, s_sz ) ;
    }
};
int test_ay_xhtml_parser(int argc,char* argv[])
{
    if( argc < 2 ) {
        std::cerr << "provide input file\n";
        return -1;
    }
         
    xhtml_cb cb;
    std::ifstream fp;
    fp.open( argv[1] );
    if( !fp.is_open() ) 
        return( std::cerr << "cant open " << argv[1] << std::endl, -1 );

    ay::xhtml_parser<xhtml_cb> parser( fp , cb );
    parser.parse();
    std::cerr << cb.counter << "," << cb.maxStackDepth << "@" << cb.longestPath << std::endl;
    return 0;
}
namespace {
int test_ay_uri(int argc,char* argv[])
{
    while(true) {
        std::string l;

        std::cout << "enter string::";
        std::getline( std::cin, l );
        
        ay::uri_parse uri;
        uri.parse( l );
        
        // std::cerr << "PREFIX:" << uri.prefix << std::endl;
        for( auto i = uri.theVec.begin(); i!= uri.theVec.end(); ++i )  {
            std::cerr << i->first << "=" << i->second << std::endl;
        }

    }
    return 0;
}

} // anonymous namespace 
#endif /// DONTCOMMENTALLOUT

namespace {

template<typename PL, typename... T>
struct VecVar {
    typedef std::vector< VecVar > Vec_t;
    typedef boost::variant< Vec_t, T... > Var_t;
    Var_t d_dta;
    PL    d_payload;

    VecVar() {}

    template <class X>
    VecVar( const X& i ) : d_dta(i) {}

    template <class X>
    VecVar( const X& i, const PL& pl  ) : d_dta(i), d_payload(pl) {}
    
    Vec_t& vec() 
    {
        if( d_dta.which() ) 
            d_dta = Vec_t();
        return boost::get<Vec_t>(d_dta);
    }
    
    template <typename X>
    const X* getPtr() const { return boost::get<X>( &d_dta ); }
    
    // CB should accept Var_t
    template <typename CB>
    void visit( const CB& cb ) const
    {
        if( const Vec_t* x = getPtr<Vec_t>() ) {
            cb(*this);
            for( const auto& i : *x ) 
                i.visit(cb);
        } else
            cb( *this );
    }
};

struct Payload {
    std::string stuff;
};

int test_variadic(int argc, char* argv[]) 
{
    typedef VecVar<std::string, int,std::string> Type;
    Type y;
    Type::Vec_t v;
    
    y.d_dta = Type::Vec_t();
    y.vec().push_back( Type(100) );
    y.vec().push_back( Type("shit") );

    v.push_back( Type("lalala") );
    v.push_back( Type(333) );
    y.vec().push_back( Type(v,"AND"));
    
    /*
    for( const auto& i: y.vec() ) {
        if( const int* tmp = i.getPtr<int>() ) {
            std::cerr << *tmp << std::endl;
        } else 
        if( const std::string* tmp = i.getPtr<std::string>() ) {
            std::cerr << *tmp << std::endl;
        }
    }
    */
    
    auto func = [&]( const Type& i) { 
        if( i.d_payload.length() ) 
            std::cerr << "PAYLOAD:" << i.d_payload<< std::endl;
        if( const int* tmp = i.getPtr<int>() ) {
            std::cerr << *tmp << std::endl;
        } else 
        if( const std::string* tmp = i.getPtr<std::string>() ) {
            std::cerr << *tmp << std::endl;
        }
    };
    y.visit( func );
    return 0;
}

}
int main(int argc, char* argv[]) {
	// testLogger();
    //testUTF8(argc,argv);
    // testUTF8Normalizer(argc,argv);
    // testStripDiacritics(argc,argv);
    // testXMLEscape(argc,argv);
    // test_ay_strcasecmp(argc,argv);
    return test_variadic(argc,argv);
}
