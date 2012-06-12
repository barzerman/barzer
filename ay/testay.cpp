/*
 * testay.cpp
 *
 *  Created on: Apr 12, 2011
 *      Author: polter
 */

#include <ay_logger.h>
#include <ay_utf8.h>
#include <ay_choose.h>
#include <ay_util_time.h>

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
void testUTF8Fancy(int argc, char* argv[]) 
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

int main(int argc, char* argv[]) {
	// testLogger();
    //testUTF8(argc,argv);
    testUTF8Fancy(argc,argv);
}
