
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_universe.h>
#include <barzer_json_output.h>
#include <boost/format.hpp>
#include <barzer_ghettodb.h>
#include <barzer_geoindex.h>


namespace barzer {

namespace {
static char g_version[] = "1.05";

inline bool isBlank(const BarzelBead &bead) {
    if (bead.isBlank()) return true;
    try {
        const BarzelBeadAtomic &atm = boost::get<BarzelBeadAtomic>(bead.getBeadData());
        const BarzerLiteral &ltrl = boost::get<BarzerLiteral>(atm.getData());
        return ltrl.isBlank();
    } catch (boost::bad_get) {
        return false;
    }
}
    struct PropCallback {
        std::ostream&   d_os;
        uint32_t        d_otherUniverseNumber; 
        json_raii raii;

        PropCallback( std::ostream& os, size_t depth ) : 
            d_os(os) ,
            d_otherUniverseNumber(0xffffffff),
            raii(os,false,depth)
        {}
        PropCallback( std::ostream& os, uint32_t u, size_t depth ) : 
            d_os(os) ,
            d_otherUniverseNumber(u),
            raii(os,false,depth)
        { }
        void operator()( const char* n, const char* v ) 
        {
            {
            std::stringstream sstr;
            ay::jsonEscape( n, sstr, "\"" );
            ay::jsonEscape( v, raii.startFieldNoindent( sstr.str().c_str() ), "\"" );
            }
            if( d_otherUniverseNumber != 0xffffffff ) 
                raii.startFieldNoindent("u") << d_otherUniverseNumber;
        }
    };

class BeadVisitor : public boost::static_visitor<bool> {
	std::ostream &os;
	const StoredUniverse &universe;
	const BarzelBead	&d_bead;
    const Barz& d_barz;
	size_t lvl;

	bool d_printTtok;
    const BarzStreamerJSON& d_streamer;
public:

    json_raii raii;
    const json_raii& getRaii() const { return raii; }
    json_raii& getRaii() { return raii; }
	BeadVisitor(std::ostream &s, const StoredUniverse &u, const BarzelBead& b, const Barz& barz, const BarzStreamerJSON& streamer, size_t depth ) : 
		os(s), universe(u), d_bead(b),d_barz(barz), lvl(0), d_printTtok(true), d_streamer(streamer), raii(s,false,depth)
    {
        if( b.isComplexAtomicType() && b.getConfidence()!= BarzelBead::CONFIDENCE_UNKNOWN ) 
            raii.startField("confidence") << b.getConfidence();
    }
    BeadVisitor( BeadVisitor& v, bool isArr):
        os(v.os),
        universe(v.universe),
        d_bead(v.d_bead),
        d_barz(v.d_barz),
        lvl(0),
        d_printTtok(v.d_printTtok),
        d_streamer(v.d_streamer),
        raii(v.os,isArr,v.raii.getDepth()+1 )
    {
    }
	void printTTokenTag( )
	{
		if( !d_printTtok ) return;
		const CTWPVec& ctoks = d_bead.getCTokens();

        bool needOffsetLengthVec = !d_streamer.checkBit( BarzResponseStreamer::BF_NO_ORIGOFFSETS );
        std::vector< std::pair<uint32_t,uint32_t> >  offsetLengthVec; 
        
        std::string lastTokBuf;
		for( CTWPVec::const_iterator ci = ctoks.begin(); ci != ctoks.end(); ++ci ) {
			const TTWPVec& ttv = ci->first.getTTokens();

			for( TTWPVec::const_iterator ti = ttv.begin(); ti!= ttv.end() ; ++ti ) {
				const TToken& ttok = ti->first;
                if( lastTokBuf == ttok.buf ) 
                    continue;
                else
                    lastTokBuf = ttok.buf.c_str();

				if( ttok.buf.length() ) {
                    if( needOffsetLengthVec ) 
                        offsetLengthVec.push_back( ttok.getOrigOffsetAndLength() );
				}
			}
		}	
        std::string srcTok = d_barz.getBeadSrcTok( d_bead );
		ay::jsonEscape( srcTok.c_str(), raii.startField("src"), "\"" );

        if( needOffsetLengthVec ) {
            raii.startField( "origmarkup" );

            if( needOffsetLengthVec ) {
                std::stringstream xstr;
                for( auto i = offsetLengthVec.begin(); i!= offsetLengthVec.end(); ++i ) {
                    if(i!= offsetLengthVec.begin() )
                        xstr << ";";
                    std::pair< size_t, size_t > pp = d_barz.getGlyphFromOffsets(i->first,i->second);
                    xstr << pp.first << "," << pp.second;
                }
                os << "\"" << xstr.str() << "\"";
            }
        }
	}
	bool operator()(const BarzerLiteral &data) {
		//AYLOG(DEBUG) << "BarzerLiteral";
		switch(data.getType()) {
		case BarzerLiteral::T_STRING:
		case BarzerLiteral::T_COMPOUND:
			{
				const char *cstr = universe.getStringPool().resolveId(data.getId());
                if( cstr ) {
                    if( ispunct(*cstr) ) {
                        raii.addKeyVal( "type", "punct" );
                        ay::jsonEscape(cstr, raii.startField( "value"), "\"");
                    } else {
                        raii.addKeyVal( "type", "token" );
                        ay::jsonEscape(cstr, raii.startField( "value"), "\"");
                    }
                }
			}
			break;
		case BarzerLiteral::T_STOP:
			{
				if (data.getId() == INVALID_STORED_ID) {
                    raii.addKeyVal( "type", "fluff" ) ;
				} else {
					const char *cstr = universe.getStringPool().resolveId(data.getId());
					if (cstr) {
                        raii.addKeyVal( "type", "fluff" ) ;
                        ay::jsonEscape(cstr, raii.startField( "value"), "\"");
					} 
				}
			}
			break;
		case BarzerLiteral::T_PUNCT:
			{ // need to somehow make this localised
				const char str[] = { (char)data.getId(), '\0' };
                raii.addKeyVal( "type", "punct" ) ;
                ay::jsonEscape(str, raii.startField( "value"), "\"");
			}
			break;
		case BarzerLiteral::T_BLANK:
		    return false;
			break;
		default:
            raii.addKeyVal( "type", "error" );
            ay::jsonEscape( "unknown literal type", raii.startField( "value" ), "\"" );
		}
		return true;
	}
	bool operator()(const BarzerNone &data) {
		return false;
	}
	bool operator()(const BarzerString &data) {
        raii.addKeyVal( "type", ( data.isFluff() ? "fluff" : "token") );
        ay::jsonEscape( data.getStr().c_str(), raii.startField( "value" ), "\"" );
	    return true;
	}
	bool operator()(const BarzerNumber &data) {
        raii.addKeyVal( "type", "number" );
		const char *type =  data.isReal() ? "real" : (data.isInt() ? "int" : "NaN");
        data.print( raii.startField( "value" ) );
		return true;
	}
	bool operator()(const BarzerDate &data) {
        raii.addKeyVal( "type", "date" );
        raii.startFieldNoindent( "value" ) << 
                 "\"" <<
                std::setw(2) << std::setfill('0') <<  (int)data.getYear() <<  "-" <<
                std::setw(2) << std::setfill('0') << (int)data.getMonth() << "-" <<
                std::setw(2) << std::setfill('0') << (int)data.getDay() << "\"";
		return true;
	}
	bool operator()(const BarzerTimeOfDay &data) {
        raii.addKeyVal( "type", "time" );
        raii.startFieldNoindent( "value" ) << "\"" <<
            std::setw(2) << std::setfill('0') << (int)data.getHH() << ":" <<
            std::setw(2) << std::setfill('0') << (int)data.getMM() << ":" << 
            std::setw(2) << std::setfill('0') << (int)data.getSS()  << "\"" ;
		return true;
	}
	bool operator()(const BarzerDateTime &data) {
		// tag_raii td(os, "timestamp");
        raii.addKeyVal( "type", "timestamp" );

        if( data.date.isValid() ) 
            raii.startFieldNoindent( "date" ) << 
                 "\"" <<
                std::setw(2) << std::setfill('0') <<  (int)data.date.getYear() <<  "-" <<
                std::setw(2) << std::setfill('0') << (int)data.date.getMonth() << "-" <<
                std::setw(2) << std::setfill('0') << (int)data.date.getDay() << "\"";

        if( data.timeOfDay.isValid() )
            raii.startFieldNoindent( "time" ) <<   "\"" <<
            std::setw(2) << std::setfill('0') << (int)data.timeOfDay.getHH() << ":" << 
            std::setw(2) << std::setfill('0') << (int)data.timeOfDay.getMM() << ":" << 
            std::setw(2) << std::setfill('0') << (int)data.timeOfDay.getSS() << "\"";
		return true;
	}

	struct RangeVisitor : boost::static_visitor<>  {
		BeadVisitor& bvis;
        json_raii&    raii;
		RangeVisitor( BeadVisitor& bv, json_raii& ri  ) : bvis(bv), raii(ri) {}

		void operator() ( const BarzerRange::Integer& i ) 
		{
            raii.startFieldNoindent( "lo" ) << i.first;
            raii.startFieldNoindent( "hi" ) << i.second;
		}
		void operator() ( const BarzerRange::Real& i ) 
		{
            raii.startFieldNoindent( "lo" ) << i.first;
            raii.startFieldNoindent( "hi" ) << i.second;
		}
		void operator() ( const BarzerRange::Literal& i ) 
		{
            if( !i.first.isNull() ) {
				if( const char *cstr = bvis.universe.getStringPool().resolveId(i.first.getId()) )
                    ay::jsonEscape( cstr, raii.startFieldNoindent("lo"), "\"" );
            }
            if( !i.second.isNull() ) {
				if( const char *cstr = bvis.universe.getStringPool().resolveId(i.second.getId()) )
                    ay::jsonEscape( cstr, raii.startFieldNoindent("hi"), "\"" );
            }
		}
		void operator() ( const BarzerRange::None& i ) {}

		template <typename T>
		void operator()( const T& d ) 
		{ 
            raii.startField( "lo" );

            {
            BeadVisitor visClone(bvis,false);
			visClone( d.first );
            }

            raii.startField( "hi" );
            {
            BeadVisitor visClone(bvis,false);
			visClone( d.second );
            }
		}
	};
    bool printRange( const BarzerRange &data, json_raii* xraii = 0 )
    {
        json_raii* theRaii = ( xraii ? xraii: &raii );

        theRaii->addKeyVal( "type", "range" );
        theRaii->addKeyValNoIndent( "rangetype", data.jsonGetTypeName() );
        theRaii->addKeyValNoIndent( "order", (data.isAsc() ? "ASC" :  "DESC") );
        if( !data.isFull() )
            theRaii->addKeyValNoIndent( "opt", (data.isNoHi() ? "NOHI" : "NOLO" ) );
		if (!data.isBlank()) {
            // theRaii->startField( "data" );
            // json_raii rangeRaii( os , true, theRaii->getDepth()+1 );

			RangeVisitor v( *this, *theRaii );
			boost::apply_visitor( v, data.dta );
		}
		return true;
        
    }
	bool operator()(const BarzerRange &data) {
        return printRange(data);
	}
    // popRank is for entities only
	void printEntity(const BarzerEntity &euid, bool needType, json_raii* otherRaii =0, const int *popRank = 0, const double *coverage = 0 ) 
    {
        json_raii* theRaii = ( otherRaii ? otherRaii: &raii );
        if( needType )
		    theRaii->startField("type") << "\"entity\"";
        if( popRank )
		    theRaii->startField("rank") << *popRank ;
        if( coverage) 
		    theRaii->startField("cover") << *coverage ;
        const char* tokname = universe.getGlobalPools().internalString_resolve(euid.tokId);

        if( !d_streamer.isSimplified() )
            theRaii->startFieldNoindent("class") << euid.eclass.ec;

        ay::jsonEscape( universe.getGlobalPools().getEntClassName(euid.eclass.ec), theRaii->startFieldNoindent("scope"), "\"" );
        {
            const std::string subclassName = universe.getSubclassName(euid.eclass);
            if( subclassName.length() )
                ay::jsonEscape( subclassName.c_str() , theRaii->startFieldNoindent("category") , "\"" );
        }
        if( !d_streamer.isSimplified() )
            theRaii->startFieldNoindent("subclass") << euid.eclass.subclass;
		if( tokname ) 
            ay::jsonEscape(tokname, theRaii->startFieldNoindent("id"), "\"" );

        const EntityData::EntProp* edata = universe.getEntPropData( euid );
        if( edata ) { 
            if( edata->canonicName.length() ) 
                ay::jsonEscape( edata->canonicName.c_str(), theRaii->startFieldNoindent("name"), "\"" );
            theRaii->startFieldNoindent("rel") << edata->relevance;
        }


        if( const BarzerGeo* geo = universe.getGeo() ) {
            StoredEntityId entId = universe.getDtaIdx().getEntIdByEuid( euid );
            if( entId != 0xffffffff ) {
                if( const GeoIndex_t::Point* point = geo->getCoords( entId ) ) {
                    json_raii xraii( theRaii->startField("geo"), true, theRaii->getDepth()+1 );
                    xraii.getFP();

                    xraii.startField("") << std::setprecision(8) << point->x();
                    xraii.startField("") << std::setprecision(8) << point->y();
                }
            }
        }
        if( d_barz.hasProperties() ) {
            json_raii propRaii( theRaii->startField("extra"), false, theRaii->getDepth()+1 );

            const Ghettodb& gd = universe.getGhettodb();  
            PropCallback callback(os, theRaii->getDepth()+2);
            gd.iterateProperties( 
                callback,
                d_barz.topicInfo.getPropNames().begin(), 
                d_barz.topicInfo.getPropNames().end(), 
                euid );

            // if current univere is not 0 and we have some 0 universe properties
            if( universe.getUserId() && d_barz.hasZeroUniverseProperties() ) {
                const StoredUniverse* zeroUniverse = universe.getGlobalPools().getUniverse(0);
                if( zeroUniverse ) {
                    const Ghettodb& zeroGd = zeroUniverse->getGhettodb();
                    PropCallback zeroUniverseCallback(os,0,theRaii->getDepth()+2);
                    zeroGd.iterateProperties(
                        zeroUniverseCallback,
                        d_barz.topicInfo.getPropNamesZeroUniverse().begin(), 
                        d_barz.topicInfo.getPropNamesZeroUniverse().end(), 
                        euid );
                }
            }
        } 
	}



	// not sure how to properly deconstruct this yet
	bool operator()(const BarzerEntityList &data) {
        if( data.getList().size() == 1 ) {
            printEntity(data.getList()[0],true,0);
        } else {
            raii.startField( "type") << "\"entlist\"";
            std::stringstream sstr;
            if( data.getClass().isValid() ) {
                raii.startField( "class") << data.getClass().ec;
                { // scope (class name)
                    if( const char* x = universe.getGlobalPools().getEntClassName(data.getClass().ec) ) 
                        ay::jsonEscape( x, raii.startField("scope"), "\"" );
                }
                raii.startField( "subclass") << data.getClass().subclass;
                { // category (subclass name)
                    const std::string subclassName = universe.getSubclassName(data.getClass());
                    if( subclassName.length() )
                        ay::jsonEscape( subclassName.c_str() , raii.startFieldNoindent("category") , "\"" );
                }
            }
         
            {
                json_raii listRaii( raii.startField("data"), true, raii.getDepth()+1 );
		        const BarzerEntityList::EList &lst = data.getList();
		        for (BarzerEntityList::EList::const_iterator li = lst.begin(); li != lst.end(); ++li) {
                    listRaii.startField("");
                    json_raii eraii( os , false, raii.getDepth()+2 );
			        printEntity(*li,false,&eraii);
		        }
            }
        }
		return true;
	}

	bool operator()(const BarzerEntity &data) {
		printEntity(data,true,0);
		return true;
	}

    template <typename T>
    std::ostream& cloneObject( json_raii& parentRaii, const char* fieldName, const T& t, bool noNewLine=false )
    {
        parentRaii.startField(fieldName,noNewLine);
        BeadVisitor clone(*this,false);
        clone( t );
        return parentRaii.getFP();
    }


	bool operator()(const BarzerEVR &data) {
        raii.addKeyVal( "type", "evr" );
        {
        raii.startField("ent");
        json_raii eraii( os , false, raii.getDepth()+1 );
        printEntity(data.getEntity(),false,&eraii);
        }

        json_raii listRaii( raii.startField("values"), true, raii.getDepth()+1 );
        for( auto i :  data.data() ) {
            listRaii.startField("");
            BeadVisitor arrVis(*this,false);
            if( !i.first.empty() ) {
               if( !i.first.empty() ) 
                   arrVis.raii.addKeyVal("tag", i.first.c_str());
            }
            boost::apply_visitor(arrVis, i.second );
        }
        return true;
    }
	bool operator()(const boost::optional<BarzerEVR> &data) {
        return (*this)( data.get() );
    }

    bool printERC( json_raii& locRaii, const BarzerERC &data ) 
    {
        locRaii.addKeyVal("type","erc") ;

        {
        locRaii.startField("ent");
        json_raii eraii( os , false, locRaii.getDepth()+1 );
        printEntity(data.getEntity(),false,&eraii);
        }
        if( data.getUnitEntity().isValid() ) {
            locRaii.startField("unit");
            json_raii eraii( os , false, locRaii.getDepth()+1 );
            printEntity(data.getUnitEntity(),false,&eraii);
        }
        {
        locRaii.startField("range");
        json_raii eraii( os , false, locRaii.getDepth()+1 );
        printRange(data.getRange(),&eraii) ; 
        }
		return true;
    }
    bool printERCExpr( json_raii& locRaii, const BarzerERCExpr &exp ) 
    {
        locRaii.addKeyVal( "type", "ercexpr" );

        {
        locRaii.startField( exp.d_type == BarzerERCExpr::T_LOGIC_OR ? "OR" : "AND" );
        json_raii x( os , true, locRaii.getDepth()+1 );
            for( auto& i :  exp.d_data ) {
                switch( i.which() ) {
                case 0: // ERC
                    {
                    x.startField("");
                    json_raii y( os , false, x.getDepth()+1 );
                    printERC( y, boost::get<BarzerERC>( i ) );
                    }
                    break;
                case 1: // ERCExpr
                    {
                    x.startField("");
                    json_raii y( os , false, x.getDepth()+1 );
                    printERCExpr( y, boost::get<BarzerERCExpr>( i ) );
                    }
                    break;
                }
            }
        }
        return true;
    }
	bool operator()(const BarzerERCExpr &exp) {
        return printERCExpr( raii, exp );
	}
	bool operator()(const BarzerERC & erc) {
        return printERC( raii, erc );
	}
	bool operator()(const BarzelBeadBlank&)
	    { return false; }
	bool operator()(const BarzelBeadAtomic &data)
	{
		//AYLOG(DEBUG) << "atomic: " << data.getType();
		return boost::apply_visitor(*this, data.getData());
	}

	typedef BarzelBeadExpression BBE; // sorry!
	void printAttributes(const BBE::AttrList &lst) {
		const ay::UniqueCharPool &sp = universe.getStringPool();
		for (BBE::AttrList::const_iterator it = lst.begin(); it != lst.end(); ++it) {
			 const char *name = sp.resolveId(it->first),
					    *value = sp.resolveId(it->second);
			 if (name && value) {
				 os << " " << name <<  "=\"" << value << "\"";
			 } else {
				 AYLOG(ERROR) << "Unknown string id: "
						 	  << (name ? it->second : it-> first);
			 }
		 }
	}

	bool operator()(const BarzelBeadExpression &data)
	{

        raii.startField("type") << "\"ercexpr\"";
        raii.startField("value") << "\"UNSUPPORTED_TYPE\"";

		return true;
		//os << "</expression>";
	}
	void clear() {
		lvl = 0;
	}
};

void print_conf_leftovers( json_raii&& raii, const std::vector<std::string>& vec )
{
    std::set< std::string > tmp;
    for( const auto& i : vec ) {
        if( tmp.find(i) == tmp.end() ) {
            ay::jsonEscape( i.c_str(), raii.startField(""), "\"") ;
            tmp.insert(i);
        }
    }
}
} // anon namespace
std::ostream& BarzStreamerJSON::print_entity_fields(std::ostream& os, const BarzerEntity &euid, const StoredUniverse& universe )
{

    os << "\"cl\": " << euid.eclass.ec << ", \"sc\": " << euid.eclass.subclass;
    if( const char* tokname = universe.getGlobalPools().internalString_resolve(euid.tokId) )
        ay::jsonEscape(tokname, (os<< ", \"id\": "), "\"" );

    if( const EntityData::EntProp* edata = universe.getEntPropData( euid ) ) {
        if( !edata->canonicName.empty() )  {
            ay::jsonEscape( edata->canonicName.c_str(), (os << ", \"name\":" ), "\"" );
        }
        os << ", \"rel\": "  << edata->relevance;
    }
    return os;
}

namespace {
void printTraceInfo(json_raii& raii, const Barz& barz, const StoredUniverse& universe)
{
    std::ostream& os = raii.getFP();
    int depth = 1;
    const BarzelTrace::TraceVec &tvec = barz.barzelTrace.getTraceVec();

    const auto& gp = universe.getGlobalPools();
    BarzelTranslationTraceInfo::Vec btiVec;

    if( !tvec.empty() ) {
        json_raii traceRaii( raii.startField("trace"), true, depth );

        for( BarzelTrace::TraceVec::const_iterator ti = tvec.begin(), tend = tvec.end(); ti != tend; ++ti ) {
            json_raii mRaii(traceRaii.startField(""),false,depth+1);

            const char *name = gp.internalString_resolve( ti->tranInfo.source );
            if( !name ) name ="";

            const BELTrie* trie = gp.getTriePool().getTrie_byGlobalId(ti->globalTriePoolId) ;
            if( trie ) {
                btiVec.clear();
                ay::jsonEscape( name, mRaii.startField( "file" ), "\"" );
                mRaii.startField( "gram" ) << ti->grammarSeqNo;

                if( trie->getLinkedTraceInfo(btiVec,ti->tranId) && !btiVec.empty() ) {
                    const auto & btv0 = btiVec[0].first;
                    mRaii.startField( "stmt" ) << btv0.statementNum ;
                    mRaii.startField( "emit" ) << btv0.emitterSeqNo;

                    json_raii lmRaii( mRaii.startField("linkedmatch"), true, depth+2 );

                    for( size_t j = 1; j< btiVec.size(); ++j ) {
                        json_raii xraii(lmRaii.startField(""),false,depth+3);
                        const auto& x = btiVec[j];
                            const auto& i = x.first;
                        if( !(ti->tranInfo.statementNum== i.statementNum && i.source== ti->tranInfo.source ) ) {
                            if( const char *linkedName = gp.internalString_resolve_safe( i.source ) )
                                ay::jsonEscape( linkedName, xraii.startField( "file" ), "\"" );
                            xraii.startField( "stmt" ) << i.statementNum ;
                            xraii.startField( "emit" ) << i.emitterSeqNo;
                        }
                    }
                } else {
                    const auto & btv0 = btiVec[0].first;
                    mRaii.startField( "stmt" ) << ti->tranInfo.statementNum;
                    mRaii.startField( "emit" ) << ti->tranInfo.emitterSeqNo;
                }
            }
            if( !ti->errVec.empty()) {
                json_raii errRaii( raii.startField("error"), true, depth );
                for( std::vector< std::string >::const_iterator ei = ti->errVec.begin(); ei!= ti->errVec.end(); ++ei ) {
                    ay::jsonEscape( ei->c_str(), errRaii.startField( "" ), "\"" );
                }
            }
        }
    }  /// trace output

}

}

namespace {

inline bool stringPair_comp_less( const CToken::StringPair& l, const CToken::StringPair& r )
{ return( l.first < r.first ); }
inline bool stringPair_comp_eq( const CToken::StringPair& l, const CToken::StringPair& r )
{ return( l.first == r.first ); }

}

std::ostream& BarzStreamerJSON::print(std::ostream &os)
{
    /// BARZ header tag 
    json_raii raii( os, false, 0 );
    os << std::dec;
    raii.startField( "uid" ) << universe.getUserId() ;
    if( barz.isQueryIdValid() ) 
        raii.startField( "qid" ) << barz.getQueryId() ;

    /// end of BARZ header tag
    
	const BarzelBeadChain &bc = barz.getBeads();
	CToken::SpellCorrections spellCorrections;
	size_t curBeadNum = 1;
    { // beads context
    json_raii beadsRaii( raii.startField("beads"), true, 1 );

	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
	    if (!isBlank(*bli)) {
	        beadsRaii.startField("");

	        BeadVisitor v(os, universe, *bli, barz, *this, 2 );
	        if (boost::apply_visitor(v, bli->getBeadData())) {
	            v.printTTokenTag();
	        }
            ++curBeadNum;
	    }

		//// accumulating spell corrections in spellCorrections vector 
		const CTWPVec& ctoks = bli->getCTokens();
		for( CTWPVec::const_iterator ci = ctoks.begin(); ci != ctoks.end(); ++ci ) {
			const CToken::SpellCorrections& corr = ci->first.getSpellCorrections(); 
			spellCorrections.insert( spellCorrections.end(), corr.begin(), corr.end() );
		}
		/// end of spell corrections accumulation
	}
    } /// end of bead context 
    if( !barz.d_beni.empty() ) { // beni context
        json_raii beniRaii( raii.startField("beni"), true, 1 );
        for( const auto& i : barz.d_beni.d_entVec ) {
	        beniRaii.startField("");
            BarzelBead fakeBead;
	        BeadVisitor v(os, universe, fakeBead, barz, *this, 2 );
            v.printEntity(i.ent, true, 0, &(i.popRank), &(i.coverage));
        }
    }
    if( !barz.d_beni.zurchResultEmpty() ) { // beni zurch context
        json_raii beniRaii( raii.startField("zurch"), true, 1 );
        for( const auto& i : barz.d_beni.d_zurchEntVec ) {
	        beniRaii.startField("");
            BarzelBead fakeBead;
	        BeadVisitor v(os, universe, fakeBead, barz, *this, 2 );
            v.printEntity(i.ent, true, 0, &(i.popRank), &(i.coverage));
        }
    }
    /// printing topics 
    const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
    if( !topicMap.empty() ) {
        json_raii topicsRaii( raii.startField("topics"), true, 1 );

        BarzelBead fakeBead;
        for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
            topicsRaii.startField( "");
	        BeadVisitor v(os, universe, fakeBead, barz, *this, 2  );
            // json_raii traii( topicsRaii.startField(""),false, 2 );
            v.raii.startField( "strength" ) << topI->second ;
            v.printEntity( topI->first,true,0 );
        }
    }


    ///*
	/// printing spell corrections  if any 
	if( spellCorrections.size( ) ) {
		//os << "<spell>\n";
        json_raii spellRaii( raii.startField("spell"), true, 1 );


        CToken::SpellCorrections::const_iterator i_end = std::unique( 
            spellCorrections.begin(), spellCorrections.end(), stringPair_comp_eq
        );
		for( CToken::SpellCorrections::const_iterator i = spellCorrections.begin(); i!= i_end; ++i ) {
		    spellRaii.startField("");
		    json_raii corrRaii(spellRaii.getFP(), false, 2);
		    corrRaii.addKeyValNoIndent("type", "correction");
		    corrRaii.addKeyValNoIndent("before", i->first.data());
		    corrRaii.addKeyValNoIndent("after", i->second.data());
			//os << "<correction before=\"" << i->first << "\" after=\"" << i->second << "\"/>\n";
		}
		//os << "</spell>\n";
	}
    //*/
    if( !checkBit( BF_NOTRACE ) && !qparm.isTraceOff() ) {
        printTraceInfo(raii, barz, universe);
    }
    if( !checkBit( BF_ORIGQUERY ) ) 
        ay::jsonEscape( barz.getOrigQuestion().c_str(),  raii.startField( "query" ), "\"" );

    /// confidence
    if( universe.checkBit(UBIT_NEED_CONFIDENCE) )  {
        const BarzConfidenceData& confidenceData = barz.confidenceData; 
        if( !confidenceData.hasAnyConfidence() ) 
            return os;
    
        {
            json_raii confidenceRaii( raii.startField("confidence"), false, 1 );
            if( confidenceData.d_loCnt ) {
                std::vector< std::string > tmp ;
                confidenceData.fillString( tmp, barz.getOrigQuestion(), BarzelBead::CONFIDENCE_LOW );
                if( tmp.size() ) 
                    print_conf_leftovers( json_raii(confidenceRaii.startField("nolo"),true,2), tmp );
            }
            if( confidenceData.d_medCnt ) {
                std::vector< std::string > tmp ;
                confidenceData.fillString( tmp, barz.getOrigQuestion(), BarzelBead::CONFIDENCE_MEDIUM );
                if( tmp.size() )
                    print_conf_leftovers( json_raii(confidenceRaii.startField("nomed"),true,2), tmp );
            }
            if( confidenceData.d_hiCnt ) {
                std::vector< std::string > tmp ;
                confidenceData.fillString( tmp, barz.getOrigQuestion(), BarzelBead::CONFIDENCE_HIGH );
                if( tmp.size() )
                    print_conf_leftovers( json_raii(confidenceRaii.startField("nohi"),true,2), tmp );
            }
        } // confidence block ends
    }
    raii.addKeyVal( "ver", g_version );
	return os;
}

} //namespace barzer
