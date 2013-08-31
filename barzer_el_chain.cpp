/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_el_chain.h>
#include <barzer_el_trie.h>
#include <barzer_universe.h>
#include <barzer_server_response.h>
#include <ay/ay_char.h>
#include <ay/ay_levenshtein.h>

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

void  BarzelBead::absorbBead( const BarzelBead& bead )
{
    for( const auto& i : bead.ctokOrigVec ) {
        bool isDuplicate = false;
        for( const auto& j : ctokOrigVec ) {
            if( j.first.equal(i.first) ) {
                isDuplicate = true;
                break;
            }   
        }
        if( !isDuplicate )
            ctokOrigVec.push_back(i);
    }
    // ctokOrigVec.insert( ctokOrigVec.end(), bead.ctokOrigVec.begin(), bead.ctokOrigVec.end() );
}

int  BarzelBead::computeBeadConfidence( const StoredUniverse* u ) const
{
    if( !isComplexAtomicType() || d_confidenceBoost > 1 )
        return CONFIDENCE_HIGH;
    if( d_confidenceBoost < 0 ) 
        return CONFIDENCE_LOW;

    /// sum of squares of edit distances
    ay::LevenshteinEditDistance lev;
    size_t spellCorrSquare = 0;

    size_t sum_ed = 0, sum_corr_len=0, sum_len = 0, numTokens = ctokOrigVec.size(), numSpellCorr=0;

    for( CTWPVec::const_iterator i = ctokOrigVec.begin(); i!= ctokOrigVec.end(); ++i ) {
        const CToken& ctok = i->first;
        for( CToken::SpellCorrections::const_iterator ci = ctok.spellCorrections.begin(); ci!= ctok.spellCorrections.end(); ++ci ) {
            numSpellCorr++;
            size_t levDist = Lang::getLevenshteinDistance(lev, ci->first.c_str(), ci->second.c_str() );
            sum_ed += levDist;
            sum_corr_len += std::max( ci->first.length(), ci->second.length() );
        }
        for( TTWPVec::const_iterator ti = ctok.qtVec.begin(); ti != ctok.qtVec.end(); ++ti) {
            sum_len+= ti->first.buf.length();
        }
    }
    int conf = CONFIDENCE_HIGH;
    if( numTokens && numTokens <=2 && u ) {
        if( const BarzerEntity* ent = getEntity() ) { /// single entity
            /// single token match for bead containing entity  
            /// if canonical name == source tokens
            if( const EntityData::EntProp* edata = u->getEntPropData( *ent ) ) {
                if( !Lang::stringNoCaseCmp( getSrcTokensString(), edata->canonicName ) ) {
                    return CONFIDENCE_HIGH;
                }
            }
        } else 
        if( const BarzerEntityList* entList = getEntityList() ) { /// entity list
            /// if canonical name == source tokens for every entity in the list
            std::string srcTok = getSrcTokensString();
            bool allGood = false;
            for( const auto& e : entList->getList() ) {
                if( const EntityData::EntProp* edata = u->getEntPropData( e ) ) {
                    if( !Lang::stringNoCaseCmp( srcTok, edata->canonicName ) ) {
                        allGood= true;
                    } else {
                        allGood = false;
                        break;
                    }
                } else {
                    allGood = false;
                    break;
                }
            }
            if( allGood ) 
                return CONFIDENCE_HIGH;
        }
    }
    if( !numSpellCorr )
        return ( (numTokens>1 || d_confidenceBoost) ? CONFIDENCE_HIGH : CONFIDENCE_MEDIUM );

    if( sum_ed > 3 ) {
        return ( d_confidenceBoost? CONFIDENCE_MEDIUM: CONFIDENCE_LOW );
    } else if( sum_ed> 2 ) {
        if( sum_ed*100 < sum_len*20 ) 
            return( d_confidenceBoost? CONFIDENCE_HIGH: CONFIDENCE_MEDIUM );
        else  // 20% or more chars corrected (more than 1 out of 5)
            return( d_confidenceBoost? CONFIDENCE_MEDIUM: CONFIDENCE_LOW );
    } if( sum_ed == 2 ) {
        if( numTokens> 3) 
            return( ( sum_ed*100 < sum_len*20 && d_confidenceBoost) ? CONFIDENCE_HIGH: CONFIDENCE_MEDIUM );
        else
            return( (d_confidenceBoost && sum_len>4)? CONFIDENCE_MEDIUM: CONFIDENCE_LOW );
    } else if( numTokens>= 3 || (d_confidenceBoost&& numTokens>1) )
        return CONFIDENCE_HIGH;
    // total edit distance is 1 
    return( (d_confidenceBoost|| sum_ed*100 < sum_len*20) ? CONFIDENCE_MEDIUM: CONFIDENCE_LOW );
}

size_t BarzelBead::streamSrcTokens( std::ostream& sstr ) const
{
    size_t printed = 0;
    char lastChar = 0;
    for( CTWPVec::const_iterator ti = ctokOrigVec.begin(); ti!= ctokOrigVec.end(); ++ti ) {
        for( TTWPVec::const_iterator tt = ti->first.getTTokens().begin(); tt != ti->first.getTTokens().end(); ++tt )  {
            if( lastChar && !isspace(lastChar) && tt->first.buf.size()  && !isspace(tt->first.buf[0]) ) {
                sstr << ' ';
                ++printed;
            }
            if( tt->first.buf.size() )
                lastChar= *(tt->first.buf.rbegin());
            sstr << tt->first.buf;
            printed+= tt->first.buf.size();
        }
    }
    return printed;
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

        if( ct.stem.length() ) 
            bstr.stemStr() = ct.stem;
        else 
            bstr.stemStr().clear();

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

void BarzelBeadChain::trimBlanksFromRange( Range& rng )
{
    while( rng.first != rng.second ) {
        BeadList::iterator lastElem = rng.second;
        --lastElem;
        if( lastElem == rng.first )
            break;
        if( BarzelBeadChain::iteratorAtBlank(lastElem) )
            rng.second= lastElem;
        else if( BarzelBeadChain::iteratorAtBlank(rng.first) )
            ++rng.first;
        else
            break;
    }
}

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
    const BZSpell* bzSpell = u.getBZSpell();

    for( BeadList::iterator i = lst.begin(); i!= lst.end(); ++i ) {
        if( i->getStemStringId() != 0xffffffff ) {
            const BarzerLiteral* ltrl = i->getLiteral();
            if( ltrl ) {
                if( ltrl->getId() != 0xffffffff ) {
                    uint32_t stemStringId = i->getStemStringId();
                    if( bzSpell && bzSpell->isUsersWordById(stemStringId) ) {
                        ++numRemainingStems;
                        continue;
                    }
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


template<class T> const T* getAtomicPtr(const BarzelBeadData &dta)
{
    if( is_BarzelBeadAtomic(dta) ) {
        const BarzelBeadAtomic& atomic = boost::get<BarzelBeadAtomic>(dta);
        const BarzelBeadAtomic_var& atDta = atomic.getData();

        return boost::get<T>( &atDta );
    } else
        return 0;
}


#define DEF_BARZEL_BINDING_ARR(B) template<> barzel_binding_info<B> binding_holder_implemented<B>::array[]=
#define DEF_BARZEL_BINDING_ARR_PTRS(B) \
    template <> barzel_binding_info<B>::barzel_binding_info_cp barzel_binding_info<B>::storage_begin = ARR_BEGIN(binding_holder<B>::array);\
    template <> barzel_binding_info<B>::barzel_binding_info_cp barzel_binding_info<B>::storage_end = ARR_END(binding_holder<B>::array);

#define BARZEL_BINDING(B,N) barzel_binding_info<B>( #N, binding_holder<B>::N )
#define BARZEL_BINDING_RW(B,N) barzel_binding_info<B>( #N, binding_holder<B>::N, true )
#define BARZEL_METHOD(T,N) static bool N( BarzelBeadData& out, const StoredUniverse& universe, const T& dta, const std::vector<BarzelBeadData>* vec ) 
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
        const std::vector<BarzelBeadData>*
    )> TFunc;
    TFunc       func;
    bool        isReadWrite; // by default false
    barzel_binding_info( const char* fld, TFunc f ) : field(fld), func(f), isReadWrite(false) {}
    barzel_binding_info( const char* fld, TFunc f, bool isRw ) : field(fld), func(f), isReadWrite(isRw) {}

    typedef const  barzel_binding_info* barzel_binding_info_cp;
    static barzel_binding_info_cp storage_begin, storage_end;

    static const barzel_binding_info_cp getBindingInfo( const char* n ) 
    {
        for( barzel_binding_info_cp i = storage_begin; i <storage_end; ++i ) {
            if( !strcmp( i->field, n) ) 
                return i;
        }
        return 0;
    }

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
    static typename barzel_binding_info<T>::barzel_binding_info_cp getBindingInfo(const char*) 
        { return 0; }
    static typename barzel_binding_info<T>::TFunc* getFunc(const char*) 
        { return 0; }
    static void getAllFields( std::vector< std::string >& ) 
        { }
};

template <typename T>
struct binding_holder_implemented {
    static const typename barzel_binding_info<T>::barzel_binding_info_cp getBindingInfo(const char* n) 
        { return barzel_binding_info<T>::getBindingInfo(n); }

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
        uint32_t strId = universe.stemCorrect( str, getStr(universe,dta) );
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

/// BarzerEntityList
DECL_BARZEL_BINDING_HOLDER(BarzerEntityList) {
    BARZEL_METHOD(BarzerEntityList,ec) { RETURN_NUMBER(  (int)(dta.getClass().ec) ); }
    BARZEL_METHOD(BarzerEntityList,sc) { 
        if( !dta.getList().size() ) return true;

        
        if( !vec ) {
            RETURN_NUMBER(  (int)(dta.getList()[0].getClass().subclass) ); 
        } else {
            BarzerEntity ent(dta.getList()[0]);
            const BarzerNumber* n = getAtomicPtr<BarzerNumber>( (*vec)[0]) ;
            ent.setSubclass( n? n->getUint32(): 0xffffffff );
            out = BarzelBeadAtomic(ent);
        }

        return true;
    }
    BARZEL_METHOD(BarzerEntityList,id)
    {
        if( !dta.getList().size() ) return true;
        if( !vec ) {
            const char* tokname = universe.getGlobalPools().internalString_resolve(dta.getList()[0].tokId);
            if( tokname ) {
                uint32_t strId = universe.getGlobalPools().string_getId( tokname );
                if( strId != 0xffffffff ) 
                    RETURN_NUMBER( strId ); 
                else 
                    RETURN_ATOMIC( BarzerString(tokname)  );
            }
        } else { /// setter
            // out = BarzelBeadAtomic(dta); 
            const BarzerLiteral* ltrl = getAtomicPtr<BarzerLiteral>( (*vec)[0]) ;

            BarzerEntity ent(dta.getList()[0]);
            if( ltrl ) {
                const char* str = universe.getGlobalPools().string_resolve(ltrl->getId());
                if( str ) {
                    uint32_t internalStrId = universe.getGlobalPools().internalString_getId(str);
                    ent.setId( internalStrId );
                }
            } 
            out = BarzelBeadAtomic(ent);
        }
        return true;
    }
};

DEF_BARZEL_BINDING_ARR(BarzerEntityList)
{
    BARZEL_BINDING(BarzerEntityList,ec), // entity class
    BARZEL_BINDING_RW(BarzerEntityList,sc), // entity subclass
    BARZEL_BINDING_RW(BarzerEntityList,id)  // entity id 
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerEntityList)
/// BarzerEntity
DECL_BARZEL_BINDING_HOLDER(BarzerEntity) {
    BARZEL_METHOD(BarzerEntity,ec) { RETURN_NUMBER(  (int)(dta.getClass().ec) ); }
    BARZEL_METHOD(BarzerEntity,sc) { 
        if( !vec ) {
            RETURN_NUMBER(  (int)(dta.getClass().subclass) ); 
        } else {
            BarzerEntity ent(dta);
            const BarzerNumber* n = getAtomicPtr<BarzerNumber>( (*vec)[0]) ;
            ent.setSubclass( n? n->getUint32(): 0xffffffff );
            out = BarzelBeadAtomic(ent);
        }

        return true;
    }
    BARZEL_METHOD(BarzerEntity,id)
    {
        if( !vec ) {
            const char* tokname = universe.getGlobalPools().internalString_resolve(dta.tokId);
            if( tokname ) {
                uint32_t strId = universe.getGlobalPools().string_getId( tokname );
                if( strId != 0xffffffff ) 
                    RETURN_NUMBER( strId ); 
                else 
                    RETURN_ATOMIC( BarzerString(tokname)  );
            }
        } else { /// setter
            // out = BarzelBeadAtomic(dta); 
            const BarzerLiteral* ltrl = getAtomicPtr<BarzerLiteral>( (*vec)[0]) ;

            BarzerEntity ent(dta);
            if( ltrl ) {
                const char* str = universe.getGlobalPools().string_resolve(ltrl->getId());
                if( str ) {
                    uint32_t internalStrId = universe.getGlobalPools().internalString_getId(str);
                    ent.setId( internalStrId );
                }
            } 
            out = BarzelBeadAtomic(ent);
        }
        return true;
    }
};

DEF_BARZEL_BINDING_ARR(BarzerEntity)
{
    BARZEL_BINDING(BarzerEntity,ec), // entity class
    BARZEL_BINDING_RW(BarzerEntity,sc), // entity subclass
    BARZEL_BINDING_RW(BarzerEntity,id)  // entity id 
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerEntity)
// ERC 
DECL_BARZEL_BINDING_HOLDER(BarzerERC) {
    BARZEL_METHOD(BarzerERC,unit)      { 
        if( !vec) {
            RETURN_ATOMIC( dta.getUnitEntity() ); 
        } else { // setter 
            BarzerERC erc(dta);
            const BarzerEntity* ent = getAtomicPtr<BarzerEntity>( (*vec)[0]) ;
            if( ent ) 
                erc.setUnitEntity(*ent);
            return( out = BarzelBeadAtomic(erc), true );
        }
    }
    BARZEL_METHOD(BarzerERC,range)     { 
        if( !vec ) {
            RETURN_ATOMIC( dta.getRange() ); 
        } else {  // setter
            BarzerERC erc(dta);
            const BarzerRange* range = getAtomicPtr<BarzerRange>( (*vec)[0]) ;
            if( range ) 
                erc.setRange( *range );
            return( out = BarzelBeadAtomic(erc), true );
        }
    }
    BARZEL_METHOD(BarzerERC,entity)    { 
        if( !vec ) {
            RETURN_ATOMIC( dta.getEntity()) ; 
        } else { // setter 
            BarzerERC erc(dta);
            const BarzerEntity* ent = getAtomicPtr<BarzerEntity>( (*vec)[0]) ;
            if( ent ) 
                erc.setEntity( *ent );
            return( out = BarzelBeadAtomic(erc), true );
        }
    }
    BARZEL_METHOD(BarzerERC,ec)        { RETURN_NUMBER( ((int)(dta.getEntity().getClass().ec)) ); }
    BARZEL_METHOD(BarzerERC,sc)        { RETURN_NUMBER( ((int)(dta.getEntity().getClass().subclass)) ); }
    BARZEL_METHOD(BarzerERC,id)        {             
        const char* tokname = universe.getGlobalPools().internalString_resolve(dta.getEntity().tokId);
            if( tokname ) {
                uint32_t strId = universe.getGlobalPools().string_getId( tokname );
                if( strId != 0xffffffff ) 
                    RETURN_NUMBER( strId ); 
                else 
                    RETURN_ATOMIC( BarzerString(tokname)  );            
            }
        return true;
    }

    BARZEL_METHOD(BarzerERC,lo)     { return BarzerRangeAccessor( dta.getRange() ).getLo(out); }
    BARZEL_METHOD(BarzerERC,hi)     { return BarzerRangeAccessor( dta.getRange() ).getHi(out); }
};
DEF_BARZEL_BINDING_ARR(BarzerERC)
{
    BARZEL_BINDING(BarzerERC,ec), // entity class
    BARZEL_BINDING(BarzerERC,sc), // entity subclass
    BARZEL_BINDING(BarzerERC,id), // entity id
    BARZEL_BINDING_RW(BarzerERC,entity), // entity
    BARZEL_BINDING(BarzerERC,hi), // range hi
    BARZEL_BINDING(BarzerERC,lo), // range low
    BARZEL_BINDING_RW(BarzerERC,range), // range
    BARZEL_BINDING_RW(BarzerERC,unit), // entity
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerERC)
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

/// BarzerDateTime
DECL_BARZEL_BINDING_HOLDER( BarzerDateTime ) {
    BARZEL_METHOD(BarzerDateTime,TimeAsNumber)  { RETURN_NUMBER(dta.getTime().getLong() ); }
    BARZEL_METHOD(BarzerDateTime,h)             { RETURN_NUMBER(dta.getTime().getHH() ); }
    BARZEL_METHOD(BarzerDateTime,min)           { RETURN_NUMBER(dta.getTime().getMM() ); }
    BARZEL_METHOD(BarzerDateTime,sec)           { RETURN_NUMBER(dta.getTime().getSS() ); }
    BARZEL_METHOD(BarzerDateTime,month)         { RETURN_NUMBER(dta.getDate().getMonth()); }
    BARZEL_METHOD(BarzerDateTime,day)           { RETURN_NUMBER(dta.getDate().getDay()); }
    BARZEL_METHOD(BarzerDateTime,year)          { RETURN_NUMBER(dta.getDate().getYear()); }
    BARZEL_METHOD(BarzerDateTime,DateAsNumber)  { RETURN_NUMBER(dta.getDate().getLongDate());}
    BARZEL_METHOD(BarzerDateTime,date)          { RETURN_ATOMIC(dta.getDate());}
    BARZEL_METHOD(BarzerDateTime,time)          { RETURN_ATOMIC(dta.getTime());}
};
DEF_BARZEL_BINDING_ARR(BarzerDateTime )
{
    BARZEL_BINDING(BarzerDateTime,TimeAsNumber),
    BARZEL_BINDING(BarzerDateTime,h),
    BARZEL_BINDING(BarzerDateTime,min),
    BARZEL_BINDING(BarzerDateTime,sec),
    BARZEL_BINDING(BarzerDateTime,month) ,
    BARZEL_BINDING(BarzerDateTime,day),
    BARZEL_BINDING(BarzerDateTime,year),
    BARZEL_BINDING(BarzerDateTime,DateAsNumber),
    BARZEL_BINDING(BarzerDateTime,date),
    BARZEL_BINDING(BarzerDateTime,time)
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerDateTime)


/// RANGE
DECL_BARZEL_BINDING_HOLDER( BarzerRange ) {
    BARZEL_METHOD(BarzerRange,lo)     { return BarzerRangeAccessor( dta ).getLo(out); }
    BARZEL_METHOD(BarzerRange,hi)     { return BarzerRangeAccessor( dta ).getHi(out); }
};
DEF_BARZEL_BINDING_ARR(BarzerRange )
{
    BARZEL_BINDING(BarzerRange,lo), 
    BARZEL_BINDING(BarzerRange,hi)
};
DEF_BARZEL_BINDING_ARR_PTRS(BarzerRange)



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
    const std::vector<BarzelBeadData>* d_beadDataVec;
    bool  d_tryingToWrite;

    std::ostream& d_errStream;

    binder_visitor( std::ostream& os, BarzelBeadData& d, const StoredUniverse& universe, const char* field ) : 
        d_universe(universe), d_out(d), d_field(field), d_beadDataVec(0), d_tryingToWrite(false),d_errStream(os) {}
    binder_visitor( 
        std::ostream& os,
        BarzelBeadData& d, 
        const StoredUniverse& universe, 
        const char* field, 
        const std::vector<BarzelBeadData>* beadDataVec, 
        bool tryingToWrite ) : 
            d_universe(universe), d_out(d), d_field(field), d_beadDataVec(beadDataVec), d_tryingToWrite(tryingToWrite),d_errStream(os) {}

    /*
    template <typename T> bool operator()( const T& a ) {
        const typename barzel_binding_info<T>::TFunc* f = binding_holder<T>::getFunc(d_field);
        if( f ) 
            return (*f)(d_out, d_universe, a, d_beadDataVec );
        else
            return false; 
    }
    */
    template <typename T> bool operator()( const T& a ) {
        const typename barzel_binding_info<T>::barzel_binding_info_cp bi = binding_holder<T>::getBindingInfo(d_field);
        if( bi ) {
            if( !d_tryingToWrite || bi->isReadWrite ) {
                // const typename barzel_binding_info<T>::TFunc* f = &(bi->func);
                return (bi->func)(d_out, d_universe, a, d_beadDataVec );
            } else {
                xmlEscape( d_field, d_errStream<< "property not writable:"  );
                return false;
            }
        } else {
            xmlEscape( d_field, d_errStream<< "invalid property:"  );
        }
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
// setter 
bool BarzelBeadData_FieldBinder::operator()( BarzelBeadData& out, const char* fieldName, const std::vector<BarzelBeadData>* vec ) const
{
    if( !vec || !vec->size() )
        return false;
    
    binder_visitor vis( d_errStream, out, d_universe, fieldName, vec, true );    
    bool retVal = boost::apply_visitor(vis, d_data);
    return retVal;
}

bool BarzelBeadData_FieldBinder::operator()( BarzelBeadData& out, const char* fieldName ) const
{
    binder_visitor vis( d_errStream, out, d_universe, fieldName );    
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
		bool operator()(const BarzerERC &erc)
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

namespace {

struct BarzelBeadAtomic_var_print_visitor : public boost::static_visitor<> {
    std::ostream& fp;
    BarzelBeadAtomic_var_print_visitor( std::ostream& s ) : fp(s) {}

    template <typename T>
    void operator()( const T& v ) 
    {
        fp << v;
    }
};

} // namespace 
std::ostream& operator<<( std::ostream& fp, const BarzelBeadAtomic_var& v )
{
    print_visitor vis(fp);
    boost::apply_visitor( vis, v );

    return fp;
}
} // barzer namespace
