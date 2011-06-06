
#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>
#include <barzer_spell.h>

namespace barzer {


BarzerHunspell::BarzerHunspell( const char* affFile, const char* dictFile ) :
	d_hunspell( new Hunspell( affFile, dictFile )  )
{ }

int BarzerHunspell::addDictionary( const char* fname ) 
{
	if( d_hunspell ) 
		return d_hunspell->add_dic( fname );
	else {
		// add_dic rcode doesnt seem to be documented 
		return 666;
	}
}

BarzerHunspell::~BarzerHunspell( )
{
	if( d_hunspell ) 
		delete d_hunspell;
}

void BarzerHunspellInvoke::clear() 
{
	if( d_str_pp ) {
		d_spell.getHunspell()->free_list( &d_str_pp, d_str_pp_sz );
		d_str_pp = 0;
	}
}

/// returns number of spelling suggestions

/// returns the number of spelling suggestions 
std::pair< int, size_t> BarzerHunspellInvoke::checkSpell( const char* s )
{
	clear();
	int n = d_spell.getHunspell()->spell( s );
	if( n ) 
		return std::pair< int, size_t >( n, 0 );
	else {
		d_str_pp_sz = d_spell.getHunspell()->suggest( &d_str_pp, s );
		return std::pair< int, size_t >( n, d_str_pp_sz );
	}
}

const char* BarzerHunspellInvoke::stem( const char* s )
{
	clear();

	d_str_pp_sz = d_spell.getHunspell()->stem( &d_str_pp, s );
	if( d_str_pp_sz> 0 ) 
		return d_str_pp[0];
	else 
		return 0;
}

} // end barzer namespace 
