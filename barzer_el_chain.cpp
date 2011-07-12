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
		boost::get<BarzerLiteral>(a.dta).setString(
			(ct.isWord() && ct.getStoredTok()) ? 
			ct.getStoredTok()->getSingleTokStringId() :
			0xffffffff
		);
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
	return ( fp << ctokOrigVec );
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
namespace {
	struct BarzerERCExpr_Data_print : public boost::static_visitor<> {
		std::ostream& fp;
		BarzerERCExpr_Data_print( std::ostream& f ) : fp(f) {}

		template <typename T>
		void operator() ( const T& t ) { t.print(fp); }
	};
}
const char* BarzerERCExpr::getTypeName() const
{
	static const char* s[] = { "AND", "OR" };
	
	return ( d_type < ARR_SZ(s) ? s[d_type] : "(UNDEF)" );
}

std::ostream& BarzerERCExpr::print( std::ostream& fp ) const
{
	fp << getTypeName() ;
	if( d_eclass ) 
		fp << ":" << d_eclass ;
	fp << "{";

	BarzerERCExpr_Data_print visitor(fp);
	for( DataList::const_iterator i=d_data.begin(); i!= d_data.end(); ++i ) {
		boost::apply_visitor( visitor, *i );
	}
	return fp << "}";
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

void BarzelBeadExpression::addChild(const BarzelBeadExpression &exp) {
	if (exp.sid == ATTRLIST) {
		const AttrList &other = exp.attrs;
		attrs.insert(attrs.end(), other.begin(), other.end());
	} else {
		addChild(SubExpr(exp));
	}
}
void BarzelBeadExpression::addChild(const SubExpr &exp) {
	child.push_back(exp);
}
void BarzelBeadExpression::addAttribute(uint32_t key, uint32_t value) {
	attrs.push_back(AttrList::value_type(key, value));
}


} // barzer namespace
