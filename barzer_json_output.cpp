#include <barzer_json_output.h>

namespace barzer {

namespace {

    struct PropCallback {
        std::ostream&   d_os;
        uint32_t        d_otherUniverseNumber; 
        JSONRaii raii;

        PropCallback( std::ostream& os, size_t depth ) : 
            d_os(os) ,
            d_otherUniverseNumber(0xffffffff),
            raii(s,false,depth)
        {}
        PropCallback( std::ostream& os, uint32_t u, size_t depth ) : 
            d_os(os) ,
            d_otherUniverseNumber(u),
            raii(s,false,depth)
        { }
        void operator()( const char* n, const char* v ) 
        {
            {
            std::strstream sstr;
            ay::jsonEscape( n, sstr, "\"" );
            ay::jsonEscape( v, raii.startFieldNoindent( sstr.str().c_str() ), "\"" );
            }
            if( d_otherUniverseNumber != 0xffffffff ) 
                ay::jsonEscape( d_otherUniverseNumber, raii.startFieldNoindent("u") );
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

    JSONRaii raii;
public:
    const JSONRaii& getRaii() const { return raii; }
    JSONRaii& getRaii() { return raii; }
	BeadVisitor(std::ostream &s, const StoredUniverse &u, const BarzelBead& b, const Barz& barz, const BarzStreamerJSON& streamer, size_t depth ) : 
		os(s), universe(u), d_bead(b),d_barz(barz), lvl(0), d_printTtok(true), d_streamer(streamer), raii(s,false,depth)
    {
        if( b.isComplexAtomicType() && b.getConfidence()!= BarzelBead::CONFIDENCE_UNKNOWN ) 
            raii.startField("confidence") << b.getConfidence();
    }
    BeadVisitor( BeadVisitor& v, bool isArr)
        os(v.os),
        universe(v.universe),
        d_bead(v.d_bead),
        d_barz(v.d_barz),
        lvl(0),
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
                        ay::jsonEscape(tokStr, sstrBody);
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
                for( auto i = offsetLengthVec.begin(); i!= offsetLengthVec.end(); ++i ) {
                    if(i!= offsetLengthVec.begin() )
                        os << ";";
                    std::pair< size_t, size_t > pp = d_barz.getGlyphFromOffsets(i->first,i->second);
                    os << pp.first << "," << pp.second;
                }
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
                        raii.startField( "type" ) << "\"punct\"";
                        ay::jsonEscape(cstr, raii.startField( "value"), "\"");
                    } else {
                        raii.startField( "type" ) << "\"token\"";
                        ay::jsonEscape(cstr, raii.startField( "value"), "\"");
                    }
                }
			}
			break;
		case BarzerLiteral::T_STOP:
			{
				if (data.getId() == INVALID_STORED_ID) {
                    raii.startField( "type" ) << "\"fluff\"";
				} else {
					const char *cstr = universe.getStringPool().resolveId(data.getId());
					if (cstr) {
                        raii.startField( "type" ) << "\"fluff\"";
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
                raii.startField( "type" ) << "\"punct\"";
                ay::jsonEscape(str, raii.startField( "value"), "\"");
			}
			break;
		case BarzerLiteral::T_BLANK:
		    return false;
			break;
		default:
            raii.startField( "type" ) << "\"error\"";
            ay::jsonEscape( "unknown literal type", raii.startField( "value" ), "\"" );
		}
		return true;
	}
	bool operator()(const BarzerNone &data) {
		return false;
	}
	bool operator()(const BarzerString &data) {
        raii.startField( "type" ) << "\"token\"";
        ay::jsonEscape( data.getStr(), raii.startField( "value" ), "\"" );
	    return true;
	}
	bool operator()(const BarzerNumber &data) {
        raii.startField( "type" ) << "\"number\"";
		const char *type =  data.isReal() ? "real" : (data.isInt() ? "int" : "NaN");
        data.print( raii.startField( "value" ) );
		return true;
	}
	bool operator()(const BarzerDate &data) {
        raii.startField( "type" ) << "\"date\"";
        raii.startField( "value" ) << boost::format("\"%1%-%2%-%3%\"") 
            % (int)data.getYear() 
            % (int)data.getMonth() 
            % (int)data.getDay() ;
		return true;
	}
	bool operator()(const BarzerTimeOfDay &data) {

        os << boost::format("<time h=\"%1%\" min=\"%2%\" s=\"%3%\">") 
            % (int)data.getHH() 
            % (int)data.getMM() 
            % (int)data.getSS() ;
		printTo(os, data);
        os << "</time>";
		return true;
	}
	bool operator()(const BarzerDateTime &data) {
		// tag_raii td(os, "timestamp");
        os << "<timestamp ";
        if( data.date.isValid() ) {
            os << boost::format("y=\"%1%\" mon=\"%2%\" d=\"%3%\" ") 
            % (int)data.date.getYear() 
            % (int)data.date.getMonth() 
            % (int)data.date.getDay() ;
        }
        if( data.timeOfDay.isValid() ) {
            os << boost::format("h=\"%1%\" min=\"%2%\" s=\"%3%\"") 
                % data.timeOfDay.getHH() 
                % data.timeOfDay.getMM() 
                % data.timeOfDay.getSS() ;
        }
        os << ">";
		printTo(os, data);
        os << "</timestamp>";
		return true;
	}

	struct RangeVisitor : boost::static_visitor<>  {
		BeadVisitor& bvis;
		RangeVisitor( BeadVisitor& bv ) : bvis(bv) {}

		void operator() ( const BarzerRange::Integer& i ) 
		{
			bvis.os << "<lo><num>" << i.first << "</num></lo>";
			bvis.os << "<hi><num>" << i.second << "</num></hi>";
		}
		void operator() ( const BarzerRange::Real& i ) 
		{
			bvis.os << "<lo><num>" << i.first << "</num></lo>";
			bvis.os << "<hi><num>" << i.second << "</num></hi>";
		}
		void operator() ( const BarzerRange::None& i ) {}

		template <typename T>
		void operator()( const T& d ) 
		{ 
			bvis.os << "<lo>"; bvis.operator()( d.first ); bvis.os << "</lo>";
			bvis.os << "<hi>"; bvis.operator()( d.second ); bvis.os << "</hi>";
		}
	};
	bool operator()(const BarzerRange &data) {

	os << "<range order=\"" << (data.isAsc() ? "ASC" :  "DESC") <<  "\"";
        if( !data.isFull() ) {
            if( data.isNoHi() ) 
                os << " opt=\"NOHI\"";
            else if( data.isNoLo() ) 
                os << " opt=\"NOLO\"";
        }
		if (data.isBlank()) os << " />";
		else {
			os << ">";
			RangeVisitor v( *this );
			boost::apply_visitor( v, data.dta );
			os << "</range>";
		}
		return true;
	}

	void printEntity(const BarzerEntity &euid, bool needType, const JSONRaii* otherRaii =0 ) 
    {
        const JSONRaii* theRaii = ( otherRaii ? otherRaii: &raii );
        if( needType )
		    theRaii->startField("type") << "\"entity\"";

        const char* tokname = universe.getGlobalPools().internalString_resolve(euid.tokId);

        theRaii->startFieldNoindent("class") << euid.eclass.ec;
        theRaii->startFieldNoindent("subclass") << euid.eclass.subclass;
		if( tokname ) 
            ay::jsonEscape(tokname, theRaii->startFieldNoindent("id"), "\"" );

        const EntityData::EntProp* edata = universe.getEntPropData( euid );
        if( edata ) { 
            if( edata->canonicName.length() ) 
                ay::jsonEscape( edata->canonicName.c_str(), theRaii->startField("name"), "\"" );
            theRaii->startField("rel") << edata->relevance;
        }


        if( d_barz.hasProperties() ) {
            JSONRaii propRaii( theRaii->startField("extra"), false, theRaii->getDepth()+1 );

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
                raii.startField( "subclass") << data.getClass().subclass;
            }
         
            {
                JSONRaii listRaii( raii.startField("data"), true, raii.getDepth()+1 );
		        const BarzerEntityList::EList &lst = data.getList();
		        for (BarzerEntityList::EList::const_iterator li = lst.begin(); li != lst.end(); ++li) {
			        printEntity(*li,false,&listRaii);
		        }
            }
        }
		return true;
	}

	bool operator()(const BarzerEntity &data) {
		printEntity(data,true,0);
		return true;
	}

	bool operator()(const BarzerEntityRangeCombo &data) {
		const StoredEntityUniqId &ent = data.getEntity(),
						         &unit = data.getUnitEntity();

        raii.startField("type") << "\"erc\"";

		{ /// block 
        BeadVisitor clone(*this,false);
		clone(ent);
		if (unit.isValid()) {
            clone(unit);
		}
		clone(data.getRange());
		} // end of block
		return true;
	}
	bool operator()(const BarzerERCExpr &exp) {
        raii.startField("type") << "\"ercexpr\"";
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
} // anon namespace

std::ostream& BarzStreamerJSON::print(std::ostream &os)
{
    /// BARZ header tag 
    JSONRaii raii( os, false, 0 );
    os << std::dec;
    raii.startFeild( "uid" ) << universe.getUserId() ;
    if( barz.isQueryIdValid() ) 
        raii.startFeild( "qid" ) << barz.getQueryId() ;

    /// end of BARZ header tag
    
	const BarzelBeadChain &bc = barz.getBeads();
	CToken::SpellCorrections spellCorrections;
	size_t curBeadNum = 1;
    { // beads context
    JSONRaii beadsRaii( raii.startField("beads"), true, 1 );

	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
	    if (!isBlank(*bli)) {
	        beadsRaii.startField();

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
    /// printing topics 
    const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
    if( !topicMap.empty() ) {
        JSONRaii topicsRaii( raii.startField("topics"), true, 1 );

        BarzelBead fakeBead;
	    BeadVisitor v(os, universe, fakeBead, barz, *this, 2  );
        for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
            topicsRaii.startField( "strength" ) << topI->second ;
            v.printEntity( topI->first,true,0 );
        }
    }

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
    if( !checkBit( BF_ORIGQUERY ) ) {
        os << "\n<query>";
        ay::jsonEscape( barz.getOrigQuestion().c_str(),  os) << "</query>\n";
    }

    /// confidence
    if( universe.checkBit(StoredUniverse::UBIT_NEED_CONFIDENCE) ) 
        printConfidence(os);

	os << "</barz>\n";
	return os;
}

} //namespace barzer
