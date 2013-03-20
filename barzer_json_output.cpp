
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

    json_raii raii;
public:
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
        
        std::stringstream sstrBody;

        std::string lastTokBuf;
		for( CTWPVec::const_iterator ci = ctoks.begin(), ci_before_last = ( ctoks.size() ? ci+ctoks.size()-1: ctoks.end()); ci != ctoks.end(); ++ci ) {
			const TTWPVec& ttv = ci->first.getTTokens();

            if( ci != ctoks.begin() && ci != ci_before_last) 
                sstrBody << " ";
			for( TTWPVec::const_iterator ti = ttv.begin(); ti!= ttv.end() ; ++ti ) {
				const TToken& ttok = ti->first;
                if( lastTokBuf == ttok.buf ) 
                    continue;
                else
                    lastTokBuf = ttok.buf.c_str();

				if( ttok.buf.length() ) {
                    if( ttok.buf[0] !=' ' && (ti != ttv.begin() || ci != ctoks.begin() )) 
                        sstrBody << " ";
                    if( ttok.buf[0] !=' ' ) {
                        std::string tokStr( ttok.buf.c_str(), ttok.buf.length() );
                        sstrBody << tokStr;
                    }

                    if( needOffsetLengthVec ) 
                        offsetLengthVec.push_back( ttok.getOrigOffsetAndLength() );
				}
			}
		}	

		ay::jsonEscape( sstrBody.str().c_str(), raii.startField("src"), "\"" );
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
                        if( !ispunct(*cstr) ) {
                            ay::jsonEscape(cstr, raii.startField( "value"), "\"");
                        } 
                            
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
        raii.addKeyVal( "type", "token" );
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
        raii.startFieldNoindent( "value" ) << "\"" ;
            os << "\"" << std::setw(2) << std::setfill('0') << (int)data.getHH() << ":" << 
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
    // levDist is for entities only
	void printEntity(const BarzerEntity &euid, bool needType, json_raii* otherRaii =0, const size_t *levDist = 0, const double *coverage = 0 ) 
    {
        json_raii* theRaii = ( otherRaii ? otherRaii: &raii );
        if( needType )
		    theRaii->startField("type") << "\"entity\"";
        if( levDist )
		    theRaii->startField("lev") << "\"" << *levDist << "\"";
        if( levDist ) {
		    theRaii->startField("cover") << "\"" << *coverage << "\"";
        }
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
        (*this)( data.getEntity() );
        {
        raii.startField("ent");
        json_raii eraii( os , false, raii.getDepth()+1 );
        printEntity(data.getEntity(),false,&eraii);
        }

        if( data.data().size() ==1 && data.data().begin()->first.length()==0 ) { // default tupple
            json_raii listRaii( raii.startField("variant"), true, raii.getDepth()+1 );
            for( auto i = data.data().begin()->second.begin(), i_end=data.data().begin()->second.end(); i!= i_end; ++i ) {
                listRaii.startField("");
                BeadVisitor arrVis(*this,false);
                boost::apply_visitor(arrVis, *i );
            }
        } else {
            raii.startField( "variant" );
            json_raii tuppleRaii( os , false, raii.getDepth()+1 );
            for( auto i = data.data().begin(), i_end= data.data().end(); i!= i_end; ++i ) {
                std::string nameStr = i->first;
                if( !nameStr.length() ) { /// if theres more than one tupple then tupple name cant be blank - we force _blank_
                    nameStr= "_blank_";
                } else {
                    std::stringstream sstr;
                    ay::jsonEscape( nameStr.c_str(), sstr, "\"" );
                    nameStr = sstr.str();
                } 
                {
                    json_raii listRaii( tuppleRaii.startField(nameStr.c_str()), true, tuppleRaii.getDepth()+1 );
                    for( auto j = i->second.begin(), j_end=i->second.end(); j!= j_end; ++j )  {
                        listRaii.startField("");
                        BeadVisitor arrVis(*this,false);
                        boost::apply_visitor(arrVis, *j );
                    }
                }
            }
        }
        return true;
    }
	bool operator()(const BarzerERC &data) {
        raii.addKeyVal("type","erc") ;

        {
        raii.startField("ent");
        json_raii eraii( os , false, raii.getDepth()+1 );
        printEntity(data.getEntity(),false,&eraii);
        }
        {
        raii.startField("unit");
        json_raii eraii( os , false, raii.getDepth()+1 );
        printEntity(data.getUnitEntity(),false,&eraii);
        }
        {
        raii.startField("range");
        json_raii eraii( os , false, raii.getDepth()+1 );
        printRange(data.getRange(),&eraii) ; 
        }
		return true;
	}
	bool operator()(const BarzerERCExpr &exp) {
        raii.addKeyVal( "type", "ercexpr" );
        raii.startField("value") << "\"UNSUPPORTED_TYPE\"";
		return true;
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
            v.printEntity(i.ent, true, 0, &(i.lev));
        }
    }
    /// printing topics 
    const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
    if( !topicMap.empty() ) {
        json_raii topicsRaii( raii.startField("topics"), true, 1 );

        BarzelBead fakeBead;
	    BeadVisitor v(os, universe, fakeBead, barz, *this, 2  );
        for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
            topicsRaii.startField( "strength" ) << topI->second ;
            v.printEntity( topI->first,true,0 );
        }
    }

    /*
	/// printing spell corrections  if any 
	if( spellCorrections.size( ) ) {
		os << "<spell>\n";

        CToken::SpellCorrections::const_iterator i_end = std::unique( 
            spellCorrections.begin(), spellCorrections.end(), stringPair_comp_eq
        );
		for( CToken::SpellCorrections::const_iterator i = spellCorrections.begin(); i!= i_end; ++i ) {
			os << "<correction before=\"" << i->first << "\" after=\"" << i->second << "\"/>\n";
		}
		os << "</spell>\n";
	}
    if( !checkBit( BF_NOTRACE ) )
        printTraceInfo(os, barz, universe);
    */
    if( !checkBit( BF_ORIGQUERY ) ) 
        ay::jsonEscape( barz.getOrigQuestion().c_str(),  raii.startField( "query" ), "\"" );

    /// confidence
    if( universe.checkBit(StoredUniverse::UBIT_NEED_CONFIDENCE) )  {
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
