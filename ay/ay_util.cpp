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
		return( std::getline( *fp, str ) );
	}

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

bool is_diacritic( const char* s )
{
	return ( !isascii(s[0]) && 
		((uint8_t)s[0]== 0xc3 /* theres another codepage for the umlauted czech c OR it here */) && 
		(diacriticChar2Ascii( (uint8_t)(s[1]) )[0] !=0 ) 
	);
}

int umlautsToAscii( std::string& dest, const char* s )
{
	int numDiacritics = 0;
	for( const char* ss = s; *ss; ++ss ) {
		if( (uint8_t)*ss == 0xc3 ) {
			++numDiacritics;
			if( !*(++ss) ) return numDiacritics;
			dest.append( diacriticChar2Ascii(*ss) );
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

