#include <sstream>
#include <ay_util.h>
namespace ay {
int wstring_vec_read( WstringVec& vec, std::wistream& in )
{
	size_t sz = 0;
	while( std::getline( in,(vec.resize(vec.size()+1 ),vec.back()), L' ') )  
		++sz;

	return ( vec.resize(sz), vec.size() );
}


	InputLineReader::InputLineReader( std::istream& ss ) 
	{
		std::string inFN;
		std::getline( ss, inFN, ' ' );
		if( inFN.length() && !isspace(inFN[0]) ) {
			fp = &fs;
			fs.open( inFN.c_str() );
		} else
			fp = &std::cin;
	}

	InputLineReader::InputLineReader( const char* s ) :
		fp( (s&&*s)? &fs : &std::cin )
	{
		if( !isStdin() ) 
			fs.open( s );
	}
	bool InputLineReader::nextLine()
	{
		if( !isStdin() && !fs.is_open() ) 
			return false;
		return( std::getline( *fp, str ) ).good();
	}

const char* diacriticChar2AsciiC3( uint8_t x ) {
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
} // C3

#define CASE_C(X,Y) case X: return #Y; 

const char* diacriticChar2AsciiC4( uint8_t x ) {
switch( x ) {
CASE_C(0x80,A) // Ā
CASE_C(0x81,a) // ā
CASE_C(0x82,A) // Ă
CASE_C(0x83,a) // ă
CASE_C(0x84,A) // Ą
CASE_C(0x85,A) // ą
CASE_C(0x86,C) // Ć
CASE_C(0x87,c) // ć
CASE_C(0x88,C) // Ĉ
CASE_C(0x89,c) // ĉ
CASE_C(0x8a,C) // Ċ
CASE_C(0x8b,c) // ċ
CASE_C(0x8c,C) // Č
CASE_C(0x8d,c) // č
CASE_C(0x8e,D) // Ď
CASE_C(0x8f,d) // ď
CASE_C(0x90,D) // Đ
CASE_C(0x91,d) // đ
CASE_C(0x92,E) // Ē
CASE_C(0x93,e) // ē
CASE_C(0x94,E) // Ĕ
CASE_C(0x95,e) // ĕ
CASE_C(0x96,E) // Ė
CASE_C(0x97,e) // ė
CASE_C(0x98,E) // Ę
CASE_C(0x99,e) // ę
CASE_C(0x9a,E) // Ě
CASE_C(0x9b,e) // ě
CASE_C(0x9c,G) // Ĝ
CASE_C(0x9d,g) // ĝ
CASE_C(0x9e,G) // Ğ
CASE_C(0x9f,g) // ğ
CASE_C(0xa0,G) // Ġ
CASE_C(0xa1,g) // ġ
CASE_C(0xa2,G) // Ģ
CASE_C(0xa3,g) // ģ
CASE_C(0xa4,H) // Ĥ
CASE_C(0xa5,H) // ĥ
CASE_C(0xa6,H) // Ħ
CASE_C(0xa7,H) // ħ
CASE_C(0xa8,I) // Ĩ
CASE_C(0xa9,I) // ĩ
CASE_C(0xaa,I) // Ī
CASE_C(0xab,I) // ī
CASE_C(0xac,I) // Ĭ
CASE_C(0xad,I) // ĭ
CASE_C(0xae,I) // Į
CASE_C(0xaf,I) // į
CASE_C(0xb0,I) // İ
CASE_C(0xb1,I) // ı
CASE_C(0xb2,I) // Ĳ
CASE_C(0xb3,I) // ĳ
CASE_C(0xb4,J) // Ĵ
CASE_C(0xb5,J) // ĵ
CASE_C(0xb6,K) // Ķ
CASE_C(0xb7,K) // ķ
CASE_C(0xb8,K) // ĸ
CASE_C(0xb9,L) // Ĺ
CASE_C(0xba,L) // ĺ
CASE_C(0xbb,L) // Ļ
CASE_C(0xbc,L) // ļ
CASE_C(0xbd,L) // Ľ
CASE_C(0xbe,L) // ľ
CASE_C(0xbf,L) // Ŀ
default: return "";
}
} // c4

const char* diacriticChar2AsciiC5( uint8_t x ) {
switch( x ) {
CASE_C(0x80,L) // ŀ
CASE_C(0x81,L) // Ł
CASE_C(0x82,L) // ł
CASE_C(0x83,N) // Ń
CASE_C(0x84,N) // ń
CASE_C(0x85,N) // Ņ
CASE_C(0x86,N) // ņ
CASE_C(0x87,N) // Ň
CASE_C(0x88,N) // ň
CASE_C(0x89,N) // ŉ
CASE_C(0x8a,N) // Ŋ
CASE_C(0x8b,N) // ŋ
CASE_C(0x8c,O) // Ō
CASE_C(0x8d,O) // ō
CASE_C(0x8e,O) // Ŏ
CASE_C(0x8f,O) // ŏ
CASE_C(0x90,O) // Ő
CASE_C(0x91,O) // ő
CASE_C(0x92,O) // Œ
CASE_C(0x93,O) // œ
CASE_C(0x94,R) // Ŕ
CASE_C(0x95,R) // ŕ
CASE_C(0x96,R) // Ŗ
CASE_C(0x97,R) // ŗ
CASE_C(0x98,R) // Ř
CASE_C(0x99,R) // ř
CASE_C(0x9a,S) // Ś
CASE_C(0x9b,s) // ś
CASE_C(0x9c,S) // Ŝ
CASE_C(0x9d,s) // ŝ
CASE_C(0x9e,S) // Ş
CASE_C(0x9f,s) // ş
CASE_C(0xa0,S) // Š
CASE_C(0xa1,s) // š
CASE_C(0xa2,T) // Ţ
CASE_C(0xa3,t) // ţ
CASE_C(0xa4,T) // Ť
CASE_C(0xa5,t) // ť
CASE_C(0xa6,T) // Ŧ
CASE_C(0xa7,t) // ŧ
CASE_C(0xa8,U) // Ũ
CASE_C(0xa9,u) // ũ
CASE_C(0xaa,U) // Ū
CASE_C(0xab,u) // ū
CASE_C(0xac,U) // Ŭ
CASE_C(0xad,u) // ŭ
CASE_C(0xae,U) // Ů
CASE_C(0xaf,u) // ů
CASE_C(0xb0,U) // Ű
CASE_C(0xb1,u) // ű
CASE_C(0xb2,U) // Ų
CASE_C(0xb3,u) // ų
CASE_C(0xb4,W) // Ŵ
CASE_C(0xb5,w) // ŵ
CASE_C(0xb6,Y) // Ŷ
CASE_C(0xb7,y) // ŷ
CASE_C(0xb8,Y) // Ÿ
CASE_C(0xb9,Z) // Ź
CASE_C(0xba,z) // ź
CASE_C(0xbb,Z) // Ż
CASE_C(0xbc,z) // ż
CASE_C(0xbd,Z) // Ž
CASE_C(0xbe,z) // ž
CASE_C(0xbf,f) // ſ
default: return "";
}
} // c5
const char* diacriticChar2AsciiC6( uint8_t x ) {
switch( x ) {
CASE_C(0x80,b) // ƀ
CASE_C(0x81,B) // Ɓ
CASE_C(0x82,b) // Ƃ
CASE_C(0x83,b) // ƃ
CASE_C(0x84,b) // Ƅ
CASE_C(0x85,b) // ƅ
CASE_C(0x86,C) // Ɔ
CASE_C(0x87,C) // Ƈ
CASE_C(0x88,C) // ƈ
CASE_C(0x89,D) // Ɖ
CASE_C(0x8a,D) // Ɗ
CASE_C(0x8b,D) // Ƌ
CASE_C(0x8c,D) // ƌ
CASE_C(0x8d,Q) // ƍ
CASE_C(0x8e,E) // Ǝ
CASE_C(0x8f,e) // Ə
CASE_C(0x90,e) // Ɛ
CASE_C(0x91,f) // Ƒ
CASE_C(0x92,f) // ƒ
CASE_C(0x93,G) // Ɠ
CASE_C(0x94,G) // Ɣ
CASE_C(0x95,h) // ƕ
CASE_C(0x96,i) // Ɩ
CASE_C(0x97,I) // Ɨ
CASE_C(0x98,K) // Ƙ
CASE_C(0x99,K) // ƙ
CASE_C(0x9a,l) // ƚ
CASE_C(0x9b,l) // ƛ
CASE_C(0x9c,m) // Ɯ
CASE_C(0x9d,n) // Ɲ
CASE_C(0x9e,n) // ƞ
CASE_C(0x9f,F) // Ɵ
CASE_C(0xa0,S) // Ơ
CASE_C(0xa1,S) // ơ
CASE_C(0xa2,S) // Ƣ
CASE_C(0xa3,S) // ƣ
CASE_C(0xa4,P) // Ƥ
CASE_C(0xa5,P) // ƥ
CASE_C(0xa6,R) // Ʀ
CASE_C(0xa7,S) // Ƨ
CASE_C(0xa8,s) // ƨ
CASE_C(0xa9,S) // Ʃ
CASE_C(0xaa,r) // ƪ
CASE_C(0xab,t) // ƫ
CASE_C(0xac,T) // Ƭ
CASE_C(0xad,e) // ƭ
CASE_C(0xae,T) // Ʈ
CASE_C(0xaf,U) // Ư
CASE_C(0xb0,u) // ư
CASE_C(0xb1,o) // Ʊ
CASE_C(0xb2,N) // Ʋ
CASE_C(0xb3,Y) // Ƴ
CASE_C(0xb4,y) // ƴ
CASE_C(0xb5,Z) // Ƶ
CASE_C(0xb6,z) // ƶ
CASE_C(0xb7,Z) // Ʒ
CASE_C(0xb8,z) // Ƹ
CASE_C(0xb9,Z) // ƹ
CASE_C(0xba,z) // ƺ
CASE_C(0xbb,z) // ƻ
CASE_C(0xbc,s) // Ƽ
CASE_C(0xbd,s) // ƽ
CASE_C(0xbe,s) // ƾ
CASE_C(0xbf,R) // ƿ
default: return "";
}
} // c6
const char* diacriticChar2AsciiC7( uint8_t x ) {
switch( x ) {
CASE_C(0x84,J) // Ǆ
CASE_C(0x85,J) // ǅ
CASE_C(0x86,J) // ǆ
CASE_C(0x87,L) // Ǉ
CASE_C(0x88,L) // ǈ
CASE_C(0x89,L) // ǉ
CASE_C(0x8a,N) // Ǌ
CASE_C(0x8b,N) // ǋ
CASE_C(0x8c,N) // ǌ
CASE_C(0x8d,A) // Ǎ
CASE_C(0x8e,A) // ǎ
CASE_C(0x8f,I) // Ǐ
CASE_C(0x90,i) // ǐ
CASE_C(0x91,O) // Ǒ
CASE_C(0x92,O) // ǒ
CASE_C(0x93,U) // Ǔ
CASE_C(0x94,u) // ǔ
CASE_C(0x95,U) // Ǖ
CASE_C(0x96,u) // ǖ
CASE_C(0x97,U) // Ǘ
CASE_C(0x98,u) // ǘ
CASE_C(0x99,U) // Ǚ
CASE_C(0x9a,u) // ǚ
CASE_C(0x9b,U) // Ǜ
CASE_C(0x9c,u) // ǜ
CASE_C(0x9d,e) // ǝ
CASE_C(0x9e,A) // Ǟ
CASE_C(0x9f,a) // ǟ
CASE_C(0xa0,A) // Ǡ
CASE_C(0xa1,a) // ǡ
CASE_C(0xa2,E) // Ǣ
CASE_C(0xa3,e) // ǣ
CASE_C(0xa4,G) // Ǥ
CASE_C(0xa5,g) // ǥ
CASE_C(0xa6,G) // Ǧ
CASE_C(0xa7,g) // ǧ
CASE_C(0xa8,K) // Ǩ
CASE_C(0xa9,k) // ǩ
CASE_C(0xaa,O) // Ǫ
CASE_C(0xab,o) // ǫ
CASE_C(0xac,O) // Ǭ
CASE_C(0xad,O) // ǭ
CASE_C(0xae,Z) // Ǯ
CASE_C(0xaf,z) // ǯ
CASE_C(0xb0,j) // ǰ
CASE_C(0xb1,Z) // Ǳ
CASE_C(0xb2,Z) // ǲ
CASE_C(0xb3,z) // ǳ
CASE_C(0xb4,G) // Ǵ
CASE_C(0xb5,g) // ǵ
CASE_C(0xb6,H) // Ƕ
CASE_C(0xb7,P) // Ƿ
CASE_C(0xb8,N) // Ǹ
CASE_C(0xb9,n) // ǹ
CASE_C(0xba,A) // Ǻ
CASE_C(0xbb,a) // ǻ
CASE_C(0xbc,E) // Ǽ
CASE_C(0xbd,a) // ǽ
CASE_C(0xbe,O) // Ǿ
CASE_C(0xbf,o) // ǿ
default: return "";
}
} // c7
const char* diacriticChar2AsciiC8( uint8_t x ) {
switch( x ) {
CASE_C(0x80,A) // Ȁ
CASE_C(0x81,a) // ȁ
CASE_C(0x82,A) // Ȃ
CASE_C(0x83,a) // ȃ
CASE_C(0x84,E) // Ȅ
CASE_C(0x85,e) // ȅ
CASE_C(0x86,E) // Ȇ
CASE_C(0x87,e) // ȇ
CASE_C(0x88,I) // Ȉ
CASE_C(0x89,i) // ȉ
CASE_C(0x8a,I) // Ȋ
CASE_C(0x8b,i) // ȋ
CASE_C(0x8c,O) // Ȍ
CASE_C(0x8d,o) // ȍ
CASE_C(0x8e,O) // Ȏ
CASE_C(0x8f,o) // ȏ
CASE_C(0x90,R) // Ȑ
CASE_C(0x91,r) // ȑ
CASE_C(0x92,R) // Ȓ
CASE_C(0x93,r) // ȓ
CASE_C(0x94,U) // Ȕ
CASE_C(0x95,u) // ȕ
CASE_C(0x96,U) // Ȗ
CASE_C(0x97,u) // ȗ
CASE_C(0x98,S) // Ș
CASE_C(0x99,s) // ș
CASE_C(0x9a,T) // Ț
CASE_C(0x9b,t) // ț
CASE_C(0x9c,Z) // Ȝ
CASE_C(0x9d,z) // ȝ
CASE_C(0x9e,H) // Ȟ
CASE_C(0x9f,h) // ȟ
CASE_C(0xa0,N) // Ƞ
CASE_C(0xa1,d) // ȡ
CASE_C(0xa2,o) // Ȣ
CASE_C(0xa3,o) // ȣ
CASE_C(0xa4,Z) // Ȥ
CASE_C(0xa5,z) // ȥ
CASE_C(0xa6,A) // Ȧ
CASE_C(0xa7,a) // ȧ
CASE_C(0xa8,E) // Ȩ
CASE_C(0xa9,e) // ȩ
CASE_C(0xaa,O) // Ȫ
CASE_C(0xab,o) // ȫ
CASE_C(0xac,O) // Ȭ
CASE_C(0xad,o) // ȭ
CASE_C(0xae,O) // Ȯ
CASE_C(0xaf,o) // ȯ
CASE_C(0xb0,O) // Ȱ
CASE_C(0xb1,o) // ȱ
CASE_C(0xb2,Y) // Ȳ
CASE_C(0xb3,y) // ȳ
CASE_C(0xb4,l) // ȴ
CASE_C(0xb5,n) // ȵ
CASE_C(0xb6,t) // ȶ
CASE_C(0xb7,j) // ȷ
CASE_C(0xb8,f) // ȸ
CASE_C(0xb9,f) // ȹ
CASE_C(0xba,A) // Ⱥ
CASE_C(0xbb,C) // Ȼ
CASE_C(0xbc,c) // ȼ
CASE_C(0xbd,L) // Ƚ
CASE_C(0xbe,T) // Ⱦ
CASE_C(0xbf,S) // ȿ
default: return "";
}
} // c8
const char* diacriticChar2AsciiC9( uint8_t x ) {
switch( x ) {
CASE_C(0x80,Z) // ɀ
CASE_C(0x81,C) // Ɂ
CASE_C(0x82,c) // ɂ
CASE_C(0x83,B) // Ƀ
CASE_C(0x84,U) // Ʉ
CASE_C(0x85,A) // Ʌ
CASE_C(0x86,E) // Ɇ
CASE_C(0x87,e) // ɇ
CASE_C(0x88,J) // Ɉ
CASE_C(0x89,j) // ɉ
CASE_C(0x8a,Q) // Ɋ
CASE_C(0x8b,q) // ɋ
CASE_C(0x8c,R) // Ɍ
CASE_C(0x8d,r) // ɍ
CASE_C(0x8e,Y) // Ɏ
CASE_C(0x8f,y) // ɏ
CASE_C(0x90,a) // ɐ
CASE_C(0x91,a) // ɑ
CASE_C(0x92,a) // ɒ
CASE_C(0x93,a) // ɓ
CASE_C(0x94,c) // ɔ
CASE_C(0x95,c) // ɕ
CASE_C(0x96,p) // ɖ
CASE_C(0x97,p) // ɗ
CASE_C(0x98,e) // ɘ
CASE_C(0x99,e) // ə
CASE_C(0x9a,e) // ɚ
CASE_C(0x9b,e) // ɛ
CASE_C(0x9c,e) // ɜ
CASE_C(0x9d,e) // ɝ
CASE_C(0x9e,w) // ɞ
CASE_C(0x9f,f) // ɟ
CASE_C(0xa0,g) // ɠ
CASE_C(0xa1,g) // ɡ
CASE_C(0xa2,G) // ɢ
CASE_C(0xa3,G) // ɣ
CASE_C(0xa4,g) // ɤ
CASE_C(0xa5,h) // ɥ
CASE_C(0xa6,h) // ɦ
CASE_C(0xa7,h) // ɧ
CASE_C(0xa8,i) // ɨ
CASE_C(0xa9,i) // ɩ
CASE_C(0xaa,I) // ɪ
CASE_C(0xab,l) // ɫ
CASE_C(0xac,c) // ɬ
CASE_C(0xad,l) // ɭ
CASE_C(0xae,l) // ɮ
CASE_C(0xaf,m) // ɯ
CASE_C(0xb0,m) // ɰ
CASE_C(0xb1,m) // ɱ
CASE_C(0xb2,n) // ɲ
CASE_C(0xb3,n) // ɳ
CASE_C(0xb4,N) // ɴ
CASE_C(0xb5,o) // ɵ
CASE_C(0xb6,c) // ɶ
CASE_C(0xb7,w) // ɷ
CASE_C(0xb8,F) // ɸ
CASE_C(0xb9,r) // ɹ
CASE_C(0xba,r) // ɺ
CASE_C(0xbb,r) // ɻ
CASE_C(0xbc,r) // ɼ
CASE_C(0xbd,r) // ɽ
CASE_C(0xbe,t) // ɾ
CASE_C(0xbf,t) // ɿ
default: return "";
}
} // c9

const char*  is_diacritic( const char* s )
{
    if( (uint8_t)s[0] >= 0xc3 && (uint8_t)s[0] <= 0xc9 ) {
        const char* ss = "";
        switch( (uint8_t)s[0] ) {
        case 0xc3: ss= diacriticChar2AsciiC3( (uint8_t)(s[1]) ); break;
        case 0xc4: ss= diacriticChar2AsciiC4( (uint8_t)(s[1]) ); break;
        case 0xc5: ss= diacriticChar2AsciiC5( (uint8_t)(s[1]) ); break;
        case 0xc6: ss= diacriticChar2AsciiC6( (uint8_t)(s[1]) ); break;
        case 0xc7: ss= diacriticChar2AsciiC7( (uint8_t)(s[1]) ); break;
        case 0xc8: ss= diacriticChar2AsciiC8( (uint8_t)(s[1]) ); break;
        case 0xc9: ss= diacriticChar2AsciiC9( (uint8_t)(s[1]) ); break;
        }
        return( *ss ? ss: 0) ;
    } 
    return 0;
}


int umlautsToAscii( std::string& dest, const char* s )
{
	int numDiacritics = 0;
	for( const char* ss = s; *ss; ++ss ) {
        
        uint8_t c0 = (uint8_t)(*ss);
		if( c0 >= 0xc3 && c0 <= 0xc9 ) {
            const char* prevSS = ss;
			++numDiacritics;
			if( !*(++ss) ) return numDiacritics;
            const char* diacrStr = is_diacritic(prevSS);
            if( diacrStr )
			    dest.append( diacrStr );
		} else if( c0 == 0xe2 ) { /// wikipedia hyphen
            uint8_t c1 = (uint8_t)(ss[1]);
            uint8_t c2 = ( c1? (uint8_t)(ss[2]) : 0 );
            if( c1 == 0x80 && c2 == 0x94 ) {
                dest.push_back('-');
                ss+=2;
            } else {
			    dest.push_back( *ss );
            }
		} else {
			dest.push_back( *ss );
		}
	}
	return numDiacritics;
}

    void FileReader::setBufSz( size_t newSz ) 
    {
        if( newSz != d_buf_sz ) {
            d_buf_sz = newSz;
            d_buf = static_cast<char*>( realloc( d_buf, d_buf_sz ) );
        }
    }
    FILE* FileReader::openFile( const char* fname) 
    {
        if( d_file && d_file != stdin) {
            fclose(d_file);
            d_file=0;
        }
        if( !fname ) 
            d_file = stdin;
        else 
            d_file = fopen( fname, "r" );
        return d_file;
    }
    FileReader::FileReader( char sep, size_t bufSz): 
        d_buf_sz(bufSz), 
        d_buf( static_cast<char*>(malloc(DEFAULT_MAX_LINE_WIDTH))),
        d_file(0) ,
        d_separator('|'),
        d_comment('#')
    {}
    FileReader::~FileReader() { 
        if( d_buf ) free(d_buf); 
        if( d_file != stdin && d_file ) {
            fclose(d_file);
        }
    }
    
namespace {
    struct JunkRaii {
        std::ostream& fp;
        const char* str;
        JunkRaii( std::ostream& f, const char* s ) : fp(f), str(s) { if(str) fp << s; }
        ~JunkRaii() { if(str) fp << str; }
    };
}
std::ostream& jsonEscape(const char* tokname, std::ostream& os, const char* surroundWith )
{
    JunkRaii raii(os,surroundWith);
    for( const char* s = tokname; *s; ++s ) {
        switch( *s ) {
        case '\\': os << "\\\\"; break;
        case '"': os << "\\\""; break;
        case '\r': os << "\\r"; break;
        case '\n': os << "\\n"; break;
        case '\t': os << "\\t"; break;
        default: os << *s; break;
        }
    }

    return os;
}

inline uint8_t two_char_to_hex( char c1, char c2 )
{
    uint8_t x =0;
    if(c1 >= 'A' && c1 <= 'F') {
        x= ((uint8_t)(10+c1-'A')<<4);
    } else 
    if( c1 >= 'a' && c1 <= 'f'  ) {
        x= ((uint8_t)(10+c1-'a') <<4);
    } else 
    if( c1>='0' && c1<='9' ) {
        x= ((uint8_t)(c1-'0') << 4 );
    } else 
        return x;
    if(c2 >= 'A' && c2 <= 'F') {
        x+= (uint8_t)(10+c2-'A');
    } else 
    if( c2 >= 'a' && c2 <= 'f'  ) {
        x+= (uint8_t)(10+c2-'a');
    } else 
    if( c2>='0' && c2<='9' ) {
        x+= (uint8_t)(c2-'0');
    } 
    return x;
}


int url_encode( std::string& str, const char* s, size_t s_len ) 
{
    str.clear();
    str.reserve( s_len );
    for( const char* x = s, *s_end = s+s_len, *s_end_2 = s_end-2; x< s_end; ++x ) {
        if( *x == '%' ) {
            if( x< s_end_2 ) {
                uint8_t xx = two_char_to_hex( x[1], x[2] );
                if( xx ) 
                    str.push_back( (char)xx );
                x+=2;
            } else {
                return 1;
            }
        } else 
            str.push_back(*x);
    }
    return 0;
}

} // end of ay namespace 
#ifdef AY_UTIL_TEST_MAIN
namespace {

struct FileReaderCallback {

    int operator()( ay::FileReader& fr ) {
        const std::vector< const char* >& tok = fr.tok();
        std::cerr << tok.size() << " tokens: {";
        for( std::vector< const char* >::const_iterator i = tok.begin(); i!= tok.end(); ++i ) {
            std::cerr << '"' << *i << '"' << ',';
        }
        std::cerr << "}\n";
        return 0;
    }
};

}
int main( int argc, char* argv[] ) 
{
    const char* fname = ( argc> 1 ? argv[1] : 0 );
    ay::FileReader fr;
    FileReaderCallback cb;
    fr.readFile( cb, fname );
    return 0;
}
#endif 

