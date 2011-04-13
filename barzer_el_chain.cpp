#include <barzer_el_chain.h>

namespace barzer {

void BarzelBead::init( const CToken& ct )
{
	dta = BarzelBeadAtomic();

	BarzelBeadAtomic& atomic = boost::get< BarzelBeadAtomic >( dta );

	switch( ct.getCTokenClass() ) {
	case CLASS_UNCLASSIFIED: 
		break;
	case CLASS_BLANK: 
		boost::get<BarzerLiteral>(atomic).setBlank(); 
		break;
	case CLASS_WORD: 
		boost::get<BarzerLiteral>(atomic).setString(ct.getSingleTokStringId()); 
		break;
	case CLASS_MYSTERY_WORD: 
		boost::get<BarzerString>(atomic).setFromTTokens( ct.getTTokens() ); 
		break;
	case CLASS_NUMBER: 
		boost::get<BarzerNumber>(atomic) = ct.getNumber(); 
		break;
	case CLASS_PUNCTUATION:
		boost::get<BarzerLiteral>(atomic).setPunct(ct.getPunct());
		break;
		
	case CLASS_SPACE:
		boost::get<BarzerLiteral>(atomic).setBlank();
		break;
	}
}

namespace {

struct print_visitor : public boost::static_visitor<> 
{
	
	std::ostream& fp;
	print_visitor( std::ostream& f ) : fp(f) {}
	template <typename T> void operator()( const T& t ) const { t.print( fp ); }
};
} // anonymous namespace ends

std::ostream& BarzelBead::print( std::ostream& fp ) const
{
	boost::apply_visitor( print_visitor(fp), dta );
	fp << " ~~ ";
	return ( fp << ctokOrigVec << std::endl );
}

}
