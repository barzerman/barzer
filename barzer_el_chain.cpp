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

template <typename T>
struct barzel_binding_info {
    const char* field; 
    
    typedef boost::function<bool (
        BarzelBeadData&,
        const StoredUniverse&,
        const T& 
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

template <typename T>
struct binding_holder  {
    static typename barzel_binding_info<T>::TFunc* getFunc(const char*) 
        { return 0; }
    static void getAllFields( std::vector< std::string >& ) 
        { }
};

//// BarzerLiteral
template<>
struct binding_holder<BarzerLiteral>  {
    static const char* getStr(const StoredUniverse& universe, const BarzerLiteral& ltrl )
        { 
            const char* str = universe.getStringPool().resolveId(ltrl.getId());
            return ( str ? str : "" );
        }
    static bool length( BarzelBeadData& out, const StoredUniverse& universe, const BarzerLiteral& ltrl ) 
        { return( out = BarzelBeadAtomic( BarzerNumber( (int)(strlen(getStr(universe,ltrl))) ) ), true ); }
    static bool stem( BarzelBeadData& out, const StoredUniverse& universe, const BarzerLiteral& ltrl ) 
    {
        std::string str;
        uint32_t strId = universe.stem( str, getStr(universe,ltrl) );
        if( strId != 0xffffffff ) 
            out = BarzelBeadAtomic( BarzerLiteral(strId) );
        else 
            out = BarzelBeadAtomic( BarzerString(str.c_str()) );
        return true;
    }
    static const barzel_binding_info<BarzerLiteral>::TFunc* getFunc(const char* n) 
        { return barzel_binding_info<BarzerLiteral>::getFunc(n); }
    static barzel_binding_info<BarzerLiteral> array[];
    static void getAllFields( std::vector< std::string >& n ) { return barzel_binding_info<BarzerLiteral>::getAllFields(n); }
};

barzel_binding_info<BarzerLiteral> binding_holder<BarzerLiteral>::array[] = 
{
    barzel_binding_info<BarzerLiteral>( "length", binding_holder<BarzerLiteral>::length ),
    barzel_binding_info<BarzerLiteral>( "stem", binding_holder<BarzerLiteral>::stem ),
};
template <> barzel_binding_info<BarzerLiteral>::barzel_binding_info_cp barzel_binding_info<BarzerLiteral>::storage_begin = ARR_BEGIN(binding_holder<BarzerLiteral>::array);
template <> barzel_binding_info<BarzerLiteral>::barzel_binding_info_cp barzel_binding_info<BarzerLiteral>::storage_end = ARR_END(binding_holder<BarzerLiteral>::array);
/// end of BarzearEntity
template<>
struct binding_holder<BarzerEntity>  {
    static bool eclass( BarzelBeadData& out, const StoredUniverse& universe, const BarzerEntity& ent ) 
        { return( (out = BarzelBeadAtomic( BarzerNumber( (int)(ent.getClass().ec) )) ), true); }
    static bool esubclass( BarzelBeadData& out, const StoredUniverse& universe, const BarzerEntity& ent ) 
        { return( (out = BarzelBeadAtomic( BarzerNumber( (int)(ent.getClass().subclass) ))), true); }
    static bool id( BarzelBeadData& out, const StoredUniverse& universe, const BarzerEntity& ent ) 
    {
        const char* tokname = universe.getGlobalPools().internalString_resolve(ent.tokId);
        if( tokname ) {
            uint32_t strId = universe.getGlobalPools().string_getId( tokname );
            if( strId != 0xffffffff ) 
                return( out = BarzelBeadAtomic( BarzerNumber(strId) ), true );
            else {
                return( out = BarzelBeadAtomic( BarzerString(tokname) ), true );
            }
        }
        return true;
    }
    static void getAllFields( std::vector< std::string >& n ) { return barzel_binding_info<BarzerEntity>::getAllFields(n); }
    static const barzel_binding_info<BarzerEntity>::TFunc* getFunc(const char* n) 
        { return barzel_binding_info<BarzerEntity>::getFunc(n); }
    static barzel_binding_info<BarzerEntity> array[];
};

barzel_binding_info<BarzerEntity> binding_holder<BarzerEntity>::array[] = 
{
    barzel_binding_info<BarzerEntity>( "eclass", binding_holder<BarzerEntity>::eclass ),
    barzel_binding_info<BarzerEntity>( "esubclass", binding_holder<BarzerEntity>::esubclass ),
    barzel_binding_info<BarzerEntity>( "id", binding_holder<BarzerEntity>::id )
};
template <> barzel_binding_info<BarzerEntity>::barzel_binding_info_cp barzel_binding_info<BarzerEntity>::storage_begin = ARR_BEGIN(binding_holder<BarzerEntity>::array);
template <> barzel_binding_info<BarzerEntity>::barzel_binding_info_cp barzel_binding_info<BarzerEntity>::storage_end = ARR_END(binding_holder<BarzerEntity>::array);
/// end of BarzerEntity

struct binder_visitor : public boost::static_visitor<bool> {
    const StoredUniverse& d_universe;
    BarzelBeadData& d_out;
    const char*             d_field;
    
    binder_visitor( BarzelBeadData& d, const StoredUniverse& universe, const char* field ) : d_universe(universe), d_out(d), d_field(field) {}
    template <typename T> bool operator()( const T& a ) {
        const typename barzel_binding_info<T>::TFunc* f = binding_holder<T>::getFunc(d_field);
        if( f ) 
            return (*f)(d_out, d_universe, a );
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

} // barzer namespace
