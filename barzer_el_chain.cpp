#include <barzer_el_chain.h>
#include <barzer_el_trie.h>
#include <barzer_universe.h>

namespace barzer {
std::ostream& operator<< ( std::ostream& fp, const BarzelBeadData& x)
{
    const BarzelBeadAtomic* atomic = boost::get<BarzelBeadAtomic>( &x ); 
    if( atomic ) {
        atomic->print(fp);
    } else {
        fp << "<non-atomic>";
    }
    return fp;
}

void BarzelBead::initFromCTok( const CToken& ct )
{
	dta = BarzelBeadAtomic();
	BarzelBeadAtomic &a = boost::get< BarzelBeadAtomic >( dta );
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
	case CTokenClassInfo::CLASS_MYSTERY_WORD: {
		a.dta = BarzerString();
		BarzerString& bstr = boost::get<BarzerString>(a.dta);
        if( ct.getStemTok() ) 
            bstr.setStemStringId( ct.getStemTok()->getStringId() );

		if( ct.isSpellCorrected() ) 
			bstr.setStr( ct.correctedStr );
		else
			bstr.setFromTTokens( ct.getTTokens() );
		break;
	}
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
void BarzelBead::init( const CTWPVec::value_type& v )
{

	ctokOrigVec.clear();
	ctokOrigVec.push_back( v );

	const CToken& ct = v.first;
    initFromCTok(ct);
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

void BarzelBeadChain::collapseRangeLeft( BeadList::iterator origin, Range r ) {
	BeadList::iterator nonStringI = r.second;
	BeadList::iterator stringI = r.second;

	BeadList::iterator firstTarget = r.first;
	/// setting the string target
	if( firstTarget->isStringLiteral() ) 
		stringI = firstTarget;
	else {
		BeadList::iterator ni =firstTarget;
		for( ; ni != origin; --ni ) {
			if( ni->isStringLiteral() ) {
				stringI = ni;
				break;
			}
		}
		if( stringI == r.second && origin->isStringLiteral() ) {
			stringI = origin;
		}
	}
	// setting the non string target 
	if( !firstTarget->isStringLiteral() ) 
		nonStringI = firstTarget;
	else {
		BeadList::iterator ni = firstTarget;
		for( ; ni != origin; --ni ) {
			if( !ni->isStringLiteral() ) {
				nonStringI = ni;
				break;
			}
		}
		if( nonStringI == r.second && !origin->isStringLiteral() ) {
			nonStringI = origin;
		}
	}
	++(r.first);
	if( nonStringI == r.second ) {
		nonStringI = firstTarget;
	}
	if( stringI == r.second ) {
		stringI = firstTarget;
	}

	for( BeadList::iterator i = r.first; i!= r.second; ++i ) {
		//BeadList::iterator targetI = ( !i->isStringLiteral() ? stringI: nonStringI );
		// if( !i->isStringLiteral()  ) 
			// nonStringI->absorbBead( *i );
	}
	lst.erase( r.first, r.second );
}

size_t BarzelBeadChain::adjustStemIds( const StoredUniverse& u, const BELTrie& trie  ) 
{
    size_t numRemainingStems = 0;
    for( BeadList::iterator i = lst.begin(); i!= lst.end(); ++i ) {
        if( i->getStemStringId() != 0xffffffff ) {
            const BarzerLiteral* ltrl = i->getLiteral();
            if( ltrl ) {
                if( ltrl->getId() != 0xffffffff ) {
                    uint32_t stemStringId = i->getStemStringId();
                    const strIds_set* stridSet = trie.getStemSrcs( stemStringId );
                    if( stridSet ) {
                        uint32_t ltrlId = ltrl->getId()  ;
                        if( stridSet->find(ltrlId) != stridSet->end() ) {
                            ++numRemainingStems;
                            continue;
                        }
                    }
                    stridSet = u.getGlobalPools().getStemSrcs( stemStringId );
                    if( stridSet ) {
                        uint32_t ltrlId = ltrl->getId()  ;
                        if( stridSet->find(ltrlId) != stridSet->end() ) {
                            ++numRemainingStems;
                            continue;
                        }
                    }
                }
            } else {
                const BarzerString* bstr = i->getString();
                if( bstr && bstr->getStemStringId() == i->getStemStringId() ) {
                    ++numRemainingStems;
                    continue;
                }
            }
            i->setStemStringId( 0xffffffff );
        }
    }
    return numRemainingStems;
}

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
        lst.back().setStemStringId( i->first.getStemTokStringId() );
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

//// field name binding 

/// field binder visitor
namespace {

#define DEF_BARZEL_BINDING_ARR(B) template<> barzel_binding_info<B> binding_holder_implemented<B>::array[]=
#define DEF_BARZEL_BINDING_ARR_PTRS(B) \
    template <> barzel_binding_info<B>::barzel_binding_info_cp barzel_binding_info<B>::storage_begin = ARR_BEGIN(binding_holder<B>::array);\
    template <> barzel_binding_info<B>::barzel_binding_info_cp barzel_binding_info<B>::storage_end = ARR_END(binding_holder<B>::array);

#define BARZEL_BINDING(B,N) barzel_binding_info<B>( #N, binding_holder<B>::N )
#define BARZEL_METHOD(T,N) static bool N( BarzelBeadData& out, const StoredUniverse& universe, const T& dta, const BarzelEvalResultVec* rvec ) 
#define DECL_BARZEL_BINDING_HOLDER(B) template<> struct binding_holder<B>  : public binding_holder_implemented<B>

#define RETURN_ATOMIC( X ) return (out=BarzelBeadAtomic((X)),true)
#define RETURN_NUMBER( X ) return (out=BarzelBeadAtomic(BarzerNumber(X)),true)

// typedef std::vector< BarzelBeadData > BarzelBeadDataVec;
template <typename T>
struct barzel_binding_info {
    const char* field; 
    

    typedef boost::function<bool (
        BarzelBeadData&,
        const StoredUniverse&,
        const T&,
        const BarzelEvalResultVec*
    )> TFunc;
    TFunc       func;
    barzel_binding_info( const char* fld, TFunc f ) : field(fld), func(f) {}
    typedef const  barzel_binding_info* barzel_binding_info_cp;
    static barzel_binding_info_cp storage_begin, storage_end;
    static const TFunc* getFunc( const char* n )
    {
        for( barzel_binding_info_cp i = storage_begin; i <storage_end; ++i ) {
            if( !strcmp( i->field, n) ) 
                return (&(i->func));
        }
        return 0;
    }
    static void getAllFields( std::vector< std::string >& vec )
    {
        for( barzel_binding_info_cp i = storage_begin; i <storage_end; ++i ) {
            vec.push_back( std::string(i->field) );
        } 
    }
};
//// catcher template for un-bound classes (classes with no bindings implemented)
template <typename T>
struct binding_holder  {
    static typename barzel_binding_info<T>::TFunc* getFunc(const char*) 
        { return 0; }
    static void getAllFields( std::vector< std::string >& ) 
        { }
};

template <typename T>
struct binding_holder_implemented {
    static const typename barzel_binding_info<T>::TFunc* getFunc(const char* n) 
        { return barzel_binding_info<T>::getFunc(n); }
    static barzel_binding_info<T> array[];
    static void getAllFields( std::vector< std::string >& n ) { return barzel_binding_info<T>::getAllFields(n); }
};

//// BarzerLiteral
DECL_BARZEL_BINDING_HOLDER(BarzerLiteral) {
    static const char* getStr(const StoredUniverse& universe, const BarzerLiteral& ltrl )
        { 
            const char* str = universe.getStringPool().resolveId(ltrl.getId());
            return ( str ? str : "" );
        }
    BARZEL_METHOD(BarzerLiteral,length) 
        { return( out = BarzelBeadAtomic( BarzerNumber( (int)(strlen(getStr(universe,dta))) ) ), true ); }
    BARZEL_METHOD(BarzerLiteral,stem) 
    {
        std::string str;
        uint32_t strId = universe.stem( str, getStr(universe,dta) );
        if( strId != 0xffffffff ) 
            out = BarzelBeadAtomic( BarzerLiteral(strId) );
        else 
            out = BarzelBeadAtomic( BarzerString(str.c_str()) );
        return true;
    }
};


DEF_BARZEL_BINDING_ARR(BarzerLiteral)
{
    BARZEL_BINDING(BarzerLiteral,length),
    BARZEL_BINDING(BarzerLiteral,stem),
};

DEF_BARZEL_BINDING_ARR_PTRS(BarzerLiteral)

/// end of BarzerEntity
DECL_BARZEL_BINDING_HOLDER(BarzerEntity) {
    BARZEL_METHOD(BarzerEntity,ec) { RETURN_NUMBER(  (int)(dta.getClass().ec) ); }
    BARZEL_METHOD(BarzerEntity,sc) { RETURN_NUMBER(  (int)(dta.getClass().subclass) ); }
    BARZEL_METHOD(BarzerEntity,id)
    {
        const char* tokname = universe.getGlobalPools().internalString_resolve(dta.tokId);
        if( tokname ) {
            uint32_t strId = universe.getGlobalPools().string_getId( tokname );
            if( strId != 0xffffffff ) 
                RETURN_NUMBER( strId ); 
            else 
                RETURN_ATOMIC( BarzerString(tokname)  );
        }
        return true;
    }
};

DEF_BARZEL_BINDING_ARR(BarzerEntity)
{
    BARZEL_BINDING(BarzerEntity,ec), // entity class
    BARZEL_BINDING(BarzerEntity,sc), // entity subclass
    BARZEL_BINDING(BarzerEntity,id)  // entity id 
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerEntity)
// ERC 
DECL_BARZEL_BINDING_HOLDER(BarzerEntityRangeCombo) {
    BARZEL_METHOD(BarzerEntityRangeCombo,unit)      { RETURN_ATOMIC( dta.getUnitEntity() ); }
    BARZEL_METHOD(BarzerEntityRangeCombo,range)     { RETURN_ATOMIC( dta.getRange() ); }
    BARZEL_METHOD(BarzerEntityRangeCombo,entity)    { RETURN_ATOMIC( dta.getEntity()) ; }
    BARZEL_METHOD(BarzerEntityRangeCombo,ec)        { RETURN_NUMBER( ((int)(dta.getEntity().getClass().ec)) ); }

    BARZEL_METHOD(BarzerEntityRangeCombo,lo)     { return BarzerRangeAccessor( dta.getRange() ).getLo(out); }
    BARZEL_METHOD(BarzerEntityRangeCombo,hi)     { return BarzerRangeAccessor( dta.getRange() ).getHi(out); }
};
DEF_BARZEL_BINDING_ARR(BarzerEntityRangeCombo)
{
    BARZEL_BINDING(BarzerEntityRangeCombo,ec), // entity class
    BARZEL_BINDING(BarzerEntityRangeCombo,entity), // entity
    BARZEL_BINDING(BarzerEntityRangeCombo,hi), // range low
    BARZEL_BINDING(BarzerEntityRangeCombo,lo), // range low
    BARZEL_BINDING(BarzerEntityRangeCombo,range), // range
    BARZEL_BINDING(BarzerEntityRangeCombo,unit), // entity
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerEntityRangeCombo)
///////  BARZER DATE
DECL_BARZEL_BINDING_HOLDER(BarzerDate) {
    BARZEL_METHOD(BarzerDate,mm) { RETURN_NUMBER(dta.getMonth()); }
    BARZEL_METHOD(BarzerDate,dd) { RETURN_NUMBER(dta.getDay()); }
    BARZEL_METHOD(BarzerDate,yy) { RETURN_NUMBER(dta.getYear()); }
    BARZEL_METHOD(BarzerDate,asNumber) { RETURN_NUMBER(dta.getLongDate()); }
};
DEF_BARZEL_BINDING_ARR(BarzerDate)
{
    BARZEL_BINDING(BarzerDate,asNumber), // long date YYYYMMDD
    BARZEL_BINDING(BarzerDate,dd), // day
    BARZEL_BINDING(BarzerDate,mm), // month
    BARZEL_BINDING(BarzerDate,yy), // year
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerDate)
/// TIME OF DAY
DECL_BARZEL_BINDING_HOLDER( BarzerTimeOfDay ) {
    BARZEL_METHOD(BarzerTimeOfDay,asNumber) { RETURN_NUMBER( dta.getLong() ); }
    BARZEL_METHOD(BarzerTimeOfDay,hh) { RETURN_NUMBER( dta.getHH() ); }
    BARZEL_METHOD(BarzerTimeOfDay,mm) { RETURN_NUMBER( dta.getMM() ); }
    BARZEL_METHOD(BarzerTimeOfDay,ss) { RETURN_NUMBER( dta.getSS() ); }
};
DEF_BARZEL_BINDING_ARR(BarzerTimeOfDay )
{
    BARZEL_BINDING(BarzerTimeOfDay,asNumber), // hh
    BARZEL_BINDING(BarzerTimeOfDay,hh), // hh
    BARZEL_BINDING(BarzerTimeOfDay,mm), // minutes
    BARZEL_BINDING(BarzerTimeOfDay,ss), // seconds
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerTimeOfDay)
/* DO NOT ERASE THIS TEMPLATE - for each type youre binding
DECL_BARZEL_BINDING_HOLDER( BEADDATATYPE ) {
    BARZEL_METHOD(BarzerDate,xx) { return ( ..., true ); }
};
DEF_BARZEL_BINDING_ARR(BEADDATATYPE )
{
    BARZEL_BINDING(BEADDATATYPE,xx), // year
};
DEF_BARZEL_BINDING_ARR_PTRS(BEADDATATYPE)
*/

struct binder_visitor : public boost::static_visitor<bool> {
    const StoredUniverse& d_universe;
    BarzelBeadData& d_out;
    const char*             d_field;
    const BarzelEvalResultVec* d_evalResultVec;

    binder_visitor( BarzelBeadData& d, const StoredUniverse& universe, const char* field ) : 
        d_universe(universe), d_out(d), d_field(field), d_evalResultVec(0) {}
    binder_visitor( BarzelBeadData& d, const StoredUniverse& universe, const char* field, const BarzelEvalResultVec* evalResultVec ) : 
        d_universe(universe), d_out(d), d_field(field), d_evalResultVec(evalResultVec) {}

    template <typename T> bool operator()( const T& a ) {
        const typename barzel_binding_info<T>::TFunc* f = binding_holder<T>::getFunc(d_field);
        if( f ) 
            return (*f)(d_out, d_universe, a, d_evalResultVec );
        else
            return false; 
    }

    bool operator()( const BarzelBeadAtomic& a) 
        { return boost::apply_visitor( (*this), a.getData()); }
    
};
struct lister_visitor : public boost::static_visitor<> {
    std::vector< std::string >& d_out;
    
    lister_visitor( std::vector< std::string >& d) : d_out(d){}
    template <typename T> void operator()( const T& a ) 
        { binding_holder<T>::getAllFields(d_out); }

    void operator()( const BarzelBeadAtomic& a) 
        { boost::apply_visitor( (*this), a.getData()); }
    
};

} // anon namespace 
bool BarzelBeadData_FieldBinder::operator()( BarzelBeadData& out, const char* fieldName ) const
{
    binder_visitor vis( out, d_universe, fieldName );    
    bool retVal = boost::apply_visitor(vis, d_data);
    
    return retVal;
}

void BarzelBeadData_FieldBinder::listFields( std::vector< std::string > & out, const BarzelBeadData& d ) 
{
    lister_visitor vis( out );    
    boost::apply_visitor(vis, d);
}
/// range visitor 
namespace {

	struct RangeGetter : public boost::static_visitor<bool> {
		BarzelBeadData &result;
		bool d_isHigh;
		RangeGetter(BarzelBeadData &r, bool p ) : 
            result(r), d_isHigh(p)
        {}


		bool set(const BarzerNone&) { return false; }
		bool set(int i) 
            { return( result =  BarzelBeadAtomic( BarzerNumber(i)), true ); }
		bool set(int64_t i) 
            { return( result =  BarzelBeadAtomic( BarzerNumber(i)), true ); }
		bool set(float i) 
            { return( result =  BarzelBeadAtomic( BarzerNumber(i)), true ); }
		bool set(double i) 
            { return( result =  BarzelBeadAtomic( BarzerNumber(i)), true ); }

		template<class T> bool set(const T &v) 
			{ return( result =  BarzelBeadAtomic(v), true ); }

		template<class T> bool operator()(const T&v)
        {
             return( result =  BarzelBeadAtomic(v), true );
        }
		template<class T> bool operator()(const std::pair<T,T> &dt)
			{ return set(d_isHigh ? dt.second : dt.first); }
		bool operator()(const BarzerEntityRangeCombo &erc)
			{ return operator()(erc.getRange()); }
		bool operator()(const BarzelBeadAtomic &data)
			{ return boost::apply_visitor(*this, data.getData()); }
		bool operator()(const BarzerRange &r)
			{ return boost::apply_visitor(*this, r.getData()); }
	};
} /// end of anon namespace 

bool BarzerRangeAccessor::getHi( BarzelBeadData& out ) const
{
    if( d_range.hasHi() ) {
        RangeGetter vis( out, true );
        return boost::apply_visitor( vis, d_range.getData() );
    } else
        return ( out = BarzelBeadBlank(), false );
}
bool BarzerRangeAccessor::getLo( BarzelBeadData& out ) const
{
    if( d_range.hasLo() ) {
        RangeGetter vis( out, false );
        return boost::apply_visitor( vis, d_range.getData() );
    } else
        return ( out = BarzelBeadBlank(), false );
}

} // barzer namespace
