#include <barzer_el_chain.h>

namespace barzer {

void BarzelBead::init( const CTWPVec::value_type& v )
{

	ctokOrigVec.clear();
	ctokOrigVec.push_back( v );
	dta = BarzelBeadAtomic();

#warning "this had to fix this function. please check"

	//BarzelBeadAtomic_var& atomic = boost::get< BarzelBeadAtomic >( dta ).dta;
	BarzelBeadAtomic &a = boost::get< BarzelBeadAtomic >( dta );

	const CToken& ct = v.first;
	switch( ct.getCTokenClass() ) {
	case CTokenClassInfo::CLASS_UNCLASSIFIED: 
		break;
	case CTokenClassInfo::CLASS_BLANK: 
		//boost::get<BarzerLiteral>(atomic).setBlank();
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setBlank();
		break;
	case CTokenClassInfo::CLASS_WORD: 
		//boost::get<BarzerLiteral>(atomic).setString(ct.getStringId());
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setString(ct.getStringId());
		break;
	case CTokenClassInfo::CLASS_MYSTERY_WORD: 
		// this was throwing boost::bad_get
		//boost::get<BarzerString>(atomic).setFromTTokens( ct.getTTokens() );
		a.dta = BarzerString();
		boost::get<BarzerString>(a.dta).setFromTTokens( ct.getTTokens() );
		break;
	case CTokenClassInfo::CLASS_NUMBER:
		//boost::get<BarzerNumber>(atomic) = ct.getNumber();
		a.dta = BarzerNumber(ct.getNumber());
		break;
	case CTokenClassInfo::CLASS_PUNCTUATION:
		//boost::get<BarzerLiteral>(atomic).setPunct(ct.getPunct());
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setPunct(ct.getPunct());
		break;
		
	case CTokenClassInfo::CLASS_SPACE:
		//boost::get<BarzerLiteral>(atomic).setBlank();
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setBlank();
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

std::ostream& BarzelBeadAtomic::print( std::ostream& fp ) const
{
	print_visitor visi(fp);
	boost::apply_visitor( visi, dta );
	return fp;
}

std::ostream& BarzelBead::print( std::ostream& fp ) const
{
	boost::apply_visitor( print_visitor(fp), dta );
	fp << " ~~ ";
	return ( fp << ctokOrigVec << std::endl );
}

///// BarzelBeadChain

void BarzelBeadChain::init( const CTWPVec& cv )
{
	clear();
	for( CTWPVec::const_iterator i = cv.begin(); i!= cv.end(); ++i ) {
		lst.push_back( BarzelBead() );
		lst.back().init( *i );
	}
}


}
