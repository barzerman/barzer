#include <cstdio>
#include <string>
#include <iostream>
#include <cctype>
#include <stdint.h>
#include "umlaut.h"


const char* diacriticChar2Ascii( uint8_t x ) {
switch( x ) {
case 0x80: return "A"; // |À|
case 0x81: return "A"; // |Á|
case 0x82: return "A"; // |Â|
case 0x83: return "A"; // |Ã|
case 0x84: return "A"; //Ä|
case 0x85: return "A"; //Å|
case 0x86: return "A"; //Æ|
case 0x87: return "C"; //Ç|
case 0x88: return "E"; //È|
case 0x89: return "E"; //É|
case 0x8a: return "E"; //Ê|
case 0x8b: return "E"; //Ë|
case 0x8c: return "I"; //Ì|
case 0x8d: return "I"; //Í|
case 0x8e: return "I"; //Î|
case 0x8f: return "I"; //Ï|
case 0x90: return "D"; //Ð|
case 0x91: return "N"; //Ñ|
case 0x92: return "O"; //Ò|
case 0x93: return "O"; //Ó|
case 0x94: return "O"; //Ô|
case 0x95: return "O"; //Õ|
case 0x96: return "O"; //Ö|
case 0x97: return "x"; //×|
case 0x98: return "O"; //Ø|
case 0x99: return "U"; //Ù|
case 0x9a: return "U"; //Ú|
case 0x9b: return "U"; //Û|
case 0x9c: return "U"; //Ü|
case 0x9d: return "Y"; //Ý|
case 0x9e: return "P"; //Þ|
case 0x9f: return "ss"; //ß|
case 0xa0: return "a"; //à|
case 0xa1: return "a"; //á|
case 0xa2: return "a"; //â|
case 0xa3: return "a"; //ã|
case 0xa4: return "a"; //ä|
case 0xa5: return "a"; //å|
case 0xa6: return "a"; //æ|
case 0xa7: return "c"; //ç|
case 0xa8: return "e"; //è|
case 0xa9: return "e"; //é|
case 0xaa: return "e"; //ê|
case 0xab: return "e"; //ë|
case 0xac: return "i"; //ì|
case 0xad: return "i"; //í|
case 0xae: return "i"; //î|
case 0xaf: return "i"; //ï|
case 0xb0: return "o"; //ð|
case 0xb1: return "n"; //ñ|
case 0xb2: return "o"; //ò|
case 0xb3: return "o"; //ó|
case 0xb4: return "o"; //ô|
case 0xb5: return "o"; //õ|
case 0xb6: return "o"; //ö|
case 0xb8: return "o"; //ø|
case 0xb9: return "u"; //ù|
case 0xba: return "u"; //ú|
case 0xbb: return "u"; //û|
case 0xbc: return "u"; //ü|
case 0xbd: return "y"; //ý|
case 0xbe: return "p"; //þ|
case 0xbf: return "y"; //ÿ|
default: return "";
}
}

int umlautsToAscii( std::string& dest, const char* s )
{
	int numDiacritics = 0;
	for( const char* ss = s; *ss; ++ss ) {
		if( (uint8_t)*ss == 0xc3 ) {
			++numDiacritics;
			if( !*(++ss) ) return numDiacritics;
			const char * shit = diacriticChar2Ascii(*ss);
			dest.append( diacriticChar2Ascii(*ss) );
		} else {
			dest.push_back( *ss );
		}
	}
	return numDiacritics;
}

int main( int argc , char* argv[] ) 
{
	char buf[ 1024 ];
	while( fgets( buf, sizeof(buf) -1, stdin ) ) {
		// buf[ strlen(buf)-1 ] = 0;
		std::string str;
		umlautsToAscii( str, buf );
		std::cout << str;
	}

}
