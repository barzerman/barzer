#include <barzer_el_chain.h>

namespace barzer {

void BarzelBead::init( const CTWPVec::value_type& v )
{

	ctokOrigVec.clear();
	ctokOrigVec.push_back( v );
	dta = BarzelBeadAtomic();

	BarzelBeadAtomic &a = boost::get< BarzelBeadAtomic >( dta );

	const CToken& ct = v.first;
	switch( ct.getCTokenClass() ) {
	case CTokenClassInfo::CLASS_UNCLASSIFIED: 
		break;
	case CTokenClassInfo::CLASS_BLANK: 
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setBlank();
		break;
	case CTokenClassInfo::CLASS_WORD: 
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setString(ct.getStringId());
		break;
	case CTokenClassInfo::CLASS_MYSTERY_WORD: 
		a.dta = BarzerString();
		boost::get<BarzerString>(a.dta).setFromTTokens( ct.getTTokens() );
		break;
	case CTokenClassInfo::CLASS_NUMBER:
		a.dta = BarzerNumber(ct.getNumber());
		break;
	case CTokenClassInfo::CLASS_PUNCTUATION:
		a.dta = BarzerLiteral();
		boost::get<BarzerLiteral>(a.dta).setPunct(ct.getPunct());
		break;
		
	case CTokenClassInfo::CLASS_SPACE:
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

std::ostream& operator <<( std::ostream& fp, const BarzelBeadChain::Range& rng ) 
{
	fp << "<beadrange>";
	for(BeadList::const_iterator i = rng.first; i!= rng.second; ++i ) {
		fp << "<bead>" ;
		i->print( fp );
		fp << "</bead>" ;
	}
	fp << "</beadrange>";
	return fp;
}
///// BarzelBeadChain

void BarzelBeadChain::collapseRangeLeft( Range r ) {
	if( r.first == r.second ) 
		return;
	BarzelBead& firstBead = *(r.first);
	++(r.first);
	for( BeadList::iterator i = r.first; i!= r.second; ++i ) {
		firstBead.absorbBead( *i );
	}
	lst.erase( r.first, r.second );
}
void BarzelBeadChain::init( const CTWPVec& cv )
{
	clear();
	for( CTWPVec::const_iterator i = cv.begin(); i!= cv.end(); ++i ) {
		lst.push_back( BarzelBead() );
		lst.back().init( *i );
	}
}

struct BarzelBeadData_EQ : public boost::static_visitor<bool> {
	bool operator()(const BarzerEntityList &left, const BarzerEntityList &right) const
	    { return false; }
	bool operator()(const BarzerString &left, const BarzerString &right) const
	    { return false; }
	bool operator()(const BarzelBeadAtomic &left, const BarzelBeadAtomic &right) const
		{ return boost::apply_visitor(*this, left.getData(), right.getData()); }
	template<class T> bool operator()(const T &left, const T &right) const
		{ return ay::bcs::equal(left, right); }
	template<class T, class U> bool operator()(const T&, const U&) const { return false; }
};


//bool operator==(const BarzelBeadData &left, const BarzelBeadData &right)
bool beadsEqual(const BarzelBeadData &left, const BarzelBeadData &right)
{
	return boost::apply_visitor(BarzelBeadData_EQ(), left, right);
}


} // barzer namespace
