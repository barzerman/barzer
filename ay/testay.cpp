/*
 * testay.cpp
 *
 *  Created on: Apr 12, 2011
 *      Author: polter
 */

#include <ay_logger.h>
#include <ay_utf8.h>

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


int main() {
	// testLogger();

    std::string str;
    while(true) {
        std::cout << "enter string:";
        std::cin >> str;
        if( str == "." )
            return 0;
        ay::StrUTF8 s( str.c_str() );
        std::cerr << s << ":" << s.getGlyphCount() << std::endl;

        for( size_t i = 0; i< s.getGlyphCount(); ++i ) {
            std::cerr << "glyph[" << i << "] =" << s[i] << std::endl;
        }
        size_t lastGlyph = s.getGlyphCount()-1;

        for( size_t i = 0; i< s.getGlyphCount(); ++i ) {
            ay::StrUTF8 x(s);
            
            x.swap( i, lastGlyph-i );
            std::cerr << i << "<-->" << lastGlyph-i << "=" << x << std::endl;
        }
    }
}
