
#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>
#include <barzer_spell.h>
#include <unistd.h>
#include <ay/ay_logger.h>
#include <signal.h>

namespace barzer {

void  BarzerHunspell::initHunspell( const char* affFile, const char* dictFile ) 
{
	try {
	if( d_hunspell ) {
		delete d_hunspell;
		d_hunspell = 0;	
	}

	if( access( affFile , F_OK ) ) {
		AYLOG(ERROR) << "invalid affix file \"" <<  affFile << "\"\n";
		return;
	}
	if( access( dictFile , F_OK ) ) {
		AYLOG(ERROR) << "invalid dictionary file \"" <<  dictFile << "\"\n";
		return;
	}
	d_hunspell =  new Hunspell( affFile, dictFile )  ;
	} catch(...) {
		if( d_hunspell ) {
			delete d_hunspell;
			d_hunspell = 0;
		}
		AYLOG(ERROR) << "Hunspell initialization failed\n";
	}
}
BarzerHunspell::BarzerHunspell( const char* affFile, const char* dictFile ) :
	d_hunspell( new Hunspell( affFile, dictFile )  )
{ }

int BarzerHunspell::addDictionary( const char* fname ) 
{
	if( d_hunspell ) {
		if( access( fname , F_OK ) ) {
			AYLOG(ERROR) << "unopenable dictionary file \"" <<  fname << "\"\n";
			return 0;
		}
		return d_hunspell->add_dic( fname );
	} else {
		// add_dic rcode doesnt seem to be documented 
		AYTRACE( "hunspell is invalid" );
		return 666;
	}
}

BarzerHunspell::~BarzerHunspell( )
{
	if( d_hunspell ) 
		delete d_hunspell;
}

int BarzerHunspell::addWordsFromTextFile( const char* fname ) 
{

	if( !d_hunspell ) {
		AYTRACE( "hunspell is invalid" );
		return 0;
	}

	FILE* fp = fopen( fname, "r" );
	if( !fp ) {
		std::cerr <<"failed to open " << fname << " for reading extra words\n";
		return 0;
	}
	char buf[256*4] ;
	int numWords = 0;

	while( fgets( buf, sizeof(buf)-1, fp ) ) {
		if( !d_hunspell ) {
			fprintf( stderr, "Hunspell is broken\n" );
			break;
		}
		if( *buf == '#' ) 
			continue;
		buf[ strlen(buf) -1 ] = 0;
		if( *buf ) {
			addWord( buf );
			++numWords;
		}
	}
	fclose(fp);
	return numWords;
}
int BarzerHunspell::addWord( const char* w ) 
{
	if( d_hunspell )
		return d_hunspell->add( w );
	else 
		return 0;
}

void BarzerHunspellInvoke::clear() 
{
	Hunspell* hunspell = getHunspell();
	if( !hunspell ) return ;
	if( d_str_pp ) {
		hunspell->free_list( &d_str_pp, d_str_pp_sz );
		d_str_pp = 0;
	}
}

/// returns number of spelling suggestions

/// returns the number of spelling suggestions 
std::pair< int, size_t> BarzerHunspellInvoke::checkSpell( const char* s )
{
	Hunspell* hunspell = getHunspell();
	if( !hunspell ) {
		return std::pair< int, size_t >( 0, 0 );
	}
	clear();
	int n = hunspell->spell( s );
	if( n ) 
		return std::pair< int, size_t >( n, 0 );
	else {
		d_str_pp_sz = hunspell->suggest( &d_str_pp, s );
		return std::pair< int, size_t >( n, d_str_pp_sz );
	}
}


const char* BarzerHunspellInvoke::stem( const char* s )
{
	Hunspell* hunspell = getHunspell();
	if( !hunspell ) return 0;
	clear();

	d_str_pp_sz = hunspell->stem( &d_str_pp, s );
	if( d_str_pp_sz> 0 ) 
		return d_str_pp[0];
	else 
		return 0;
}

} // end barzer namespace 
