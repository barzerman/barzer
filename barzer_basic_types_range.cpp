#include <barzer_basic_types_range.h>
#include <barzer_el_chain.h>
#include <barzer_el_rewriter.h>
namespace barzer {

namespace { // testers

void BarzerEVR_compile_tester( )
{
    BarzerEVR evr;
    BarzerDate date;
    BarzerEntity ent;
    BarzerERC erc;
    evr.appendVar( "hello", date );
    evr.appendVar( ent );
    evr.setTagVar( "hello", erc );
}

}

namespace {
struct BarzerRange_print_visitor : public boost::static_visitor<> 
{
	
	std::ostream& fp;

	BarzerRange_print_visitor( std::ostream& f ) : fp(f) {}
	template <typename T> void operator()( const T& t ) const { 
		t.first.print( fp ); 
		fp << ",";
		t.second.print( fp ); 
	}
	void operator () ( const BarzerRange::None& t ) const
	{
		fp << "(null)";
	}
	void operator () ( const BarzerRange::Integer& t ) const
	{
		fp << t.first << "," << t.second ;
	}
	void operator () ( const BarzerRange::Real& t ) const
	{
		fp << t.first << "," << t.second ;
	}
};
} // anonymous namespace ends
std::ostream& BarzerRange::print( std::ostream& fp ) const
{ 
	fp << "Range[";
	BarzerRange_print_visitor visi( fp );
	
	boost::apply_visitor( visi, dta );

	return ( fp << "]" );
}

char BarzerRange::geXMLtAttrValueChar( ) const {

    switch( dta.which()) {
    case BarzerRange::None_TYPE: return 'n';
    case BarzerRange::Integer_TYPE: return 'i';
    case BarzerRange::Real_TYPE: return 'r';
    case BarzerRange::TimeOfDay_TYPE: return 't';
    case BarzerRange::Date_TYPE: return 'd';
    case BarzerRange::DateTime_TYPE: return 'm';
    case BarzerRange::Entity_TYPE: return 'e';
    default: return 'X';
    }

}
bool BarzerRange::scale( const BarzerNumber& n1,const BarzerNumber& n2, int mode) 
{
    if( isNumeric() ) {
        promote_toReal();
        Real* rr = boost::get<Real>( &dta );
        if( rr ) { // should always be true
            double factor1 = n1.getRealWiden();
            double factor2 = n2.getRealWiden();
            switch(mode) {
                case RANGE_MOD_SCALE_MULT:{
                    if(hasLo()) rr->first   *= factor1;
                    if(hasHi()) rr->second  *= factor2;
                } break;
                case RANGE_MOD_SCALE_DIV: if( factor1 != 0 && factor2 != 0 ) {
                    if(hasLo()) rr->first   /= factor1;
                    if(hasHi()) rr->second  /= factor2;
                } break;   
                case RANGE_MOD_SCALE_MINUS: {
                    if(hasLo()) rr->first   -= factor1;
                    if(hasHi()) rr->second  -= factor2;
                } break;
                case RANGE_MOD_SCALE_PLUS: {
                    if(hasLo()) rr->first   += factor1;
                    if(hasHi()) rr->second  += factor2;
                } break;
                default: ;//do nothing
            }

        }
        return true;
    } else 
        return false;
}    
bool BarzerRange::scale( const BarzerNumber& n, int mode  ) 
{
    if( isNumeric() ) {
        promote_toReal();
            Real* rr = boost::get<Real>( &dta );
        if( rr ) { // should always be true
            double factor = n.getRealWiden();
            switch(mode) {
                case RANGE_MOD_SCALE_MULT:{
                    rr->first   *= factor;
                    rr->second  *= factor;
                } break;
                case RANGE_MOD_SCALE_DIV: if( factor != 0 ) {
                    rr->first   /= factor;
                    rr->second  /= factor;
                } break;   
                case RANGE_MOD_SCALE_MINUS: {
                    rr->first   -= factor;
                    rr->second  -= factor;
                } break;
                case RANGE_MOD_SCALE_PLUS: {
                    rr->first   += factor;
                    rr->second  += factor;
                } break;
                default: ;//do nothing
            }

        }
        return true;
    } else 
        return false;
}
const BarzerRange& BarzerRange::promote_toReal( )
{
    if( dta.which() ==Integer_TYPE ) {
        const Integer& i= boost::get<Integer>(dta);
        dta = Data( Real( i.first, i.second ) );
    }
    return *this;
}

std::ostream& BarzerEVR::print( std::ostream& fp ) const
{
    fp << "evr{" << getEntity() << "==> ";
    for( auto& x: d_dta ) {
        fp << x.first ; // << ":" << x.second << ",";
    }
    return fp << "}";
}

void BarzerERCExpr::setLogic( const char* s ) { 
    if( !s || (tolower(s[0]) == 'a' && tolower(s[1]) == 'n' && tolower(s[2]) == 'd' && !s[3]) ) // AND 
        d_type=T_LOGIC_AND; 
    else if( (tolower(s[0]) == 'o' && tolower(s[1]) == 'r' &&!s[2] ) )
        d_type=T_LOGIC_OR;
}
namespace {

struct BarzerEVR_setter_vis : public boost::static_visitor<> {
    BarzerEVR& evr;
    const std::string& tag;
    BarzerEVR_setter_vis( BarzerEVR& evr, const std::string& tag ) : evr(evr) , tag(tag) {}


    void operator()( const BarzelBeadBlank& ) { }
    void operator()( const BarzelBeadExpression& ) { }
    void operator()( const BarzelBeadAtomic& a ) 
    { 
        boost::apply_visitor( (*this) , a.getData() );
    }
    template <typename T>
    void operator()( const T& t ) { 
        evr.setTagVar( tag, t ); 
    }
};
struct BarzerEVR_appender_vis : public boost::static_visitor<> {
    BarzerEVR& evr;
    const std::string& tag;
    BarzerEVR_appender_vis( BarzerEVR& evr, const std::string& tag ) : evr(evr) , tag(tag) {}


    void operator()( const BarzelBeadBlank& ) { }
    void operator()( const BarzelBeadExpression& ) { }
    void operator()( const BarzelBeadAtomic& a ) 
    { 
        boost::apply_visitor( (*this) , a.getData() );
    }
    template <typename T>
    void operator()( const T& t ) { 
        evr.appendVar( tag, t ); 
    }
};
} // namespace

void BarzerEVR::setTagVar( const std::string& tag, const BarzelEvalResult& r )
{
    BarzerEVR_setter_vis vis( *this, tag );
    boost::apply_visitor( vis, r.getBeadData() );
}
void BarzerEVR::appendVar( const std::string& tag, const BarzelEvalResult& r )
{
    BarzerEVR_appender_vis vis( *this, tag );
    boost::apply_visitor( vis, r.getBeadData() );
}

} // namespace barzer 
