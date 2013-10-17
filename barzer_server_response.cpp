
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_server_response.cpp
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#include <barzer_server_response.h>
#include <ay/ay_logger.h>
#include <ay/ay_raii.h>
#include <boost/format.hpp>
#include <barzer_universe.h>
#include <barzer_ghettodb.h>
#include <sstream>

namespace barzer {

// need to find an xml library for this kind of stuff
std::ostream& xmlEscape(const char *src,  std::ostream &os) {
	for(;*src != '\0'; ++src) {
		char c = (char)(*src);
		switch (c) {
		case '&': os << "&amp;"; break;
		case '<': os << "&lt;"; break;
		case '>': os << "&gt;"; break;
		case '"': os << "&quot;"; break;
		case '\'': os << "&apos;"; break;
		default: os << c;
		}
	}
	return os;
}
using ay::tag_raii;
namespace {

bool isBlank(const BarzelBead &bead) {
    if (bead.isBlank()) return true;
    try {
        const BarzelBeadAtomic &atm = boost::get<BarzelBeadAtomic>(bead.getBeadData());
        const BarzerLiteral &ltrl = boost::get<BarzerLiteral>(atm.getData());
        return ltrl.isBlank();
    } catch (boost::bad_get) {
        return false;
    }
}


inline std::ostream& xmlEscape(const std::string &src, std::ostream &os) {
	return barzer::xmlEscape(static_cast<const char*>(src.c_str()), os);
}


template<class T> std::ostream& printTo(std::ostream&, const T&);

template<>
std::ostream& printTo<BarzerDate>(std::ostream &os, const BarzerDate &dt)
{
	return (os << boost::format("%|04u|-%|02u|-%|02u|") % (int)dt.year
														% (int)dt.month
														% (int)dt.day);
}

template<>
std::ostream& printTo<BarzerTimeOfDay>(std::ostream &os,
						               const BarzerTimeOfDay &dt)
{
	return (os << boost::format("%|02d|:%|02d|:%|02d|") % (int)dt.getHH()
			 	 	 	 	 	 	 	 	 	 	 	% (int)dt.getMM()
			 	 	 	 	 	 	 	 	 	 	 	% (int)dt.getSS());
}

template<>
std::ostream& printTo<BarzerDateTime>(std::ostream &os,
						               const BarzerDateTime &dt)
{
	return printTo(printTo(os, dt.getDate()) << " ", dt.getTime());
}

namespace {
    struct PropCallback {
        std::ostream&   d_os;
        uint32_t        d_otherUniverseNumber; 

        PropCallback( std::ostream& os ) : 
            d_os(os) ,
            d_otherUniverseNumber(0xffffffff)
        {}
        PropCallback( std::ostream& os, uint32_t u ) : 
            d_os(os) ,
            d_otherUniverseNumber(u)
        {}
        void operator()( const char* n, const char* v ) 
        {
            if( d_otherUniverseNumber == 0xffffffff ) 
                d_os << "<nv n=\"" << n << "\" v=\"" << v << "\"/>";
            else
                d_os << "<nv n=\"" << n << "\" v=\"" << v << "\" u=\"" << d_otherUniverseNumber << "\"/>";
        }
    };
}
class BeadVisitor : public boost::static_visitor<bool> {
	std::ostream &os;
	const StoredUniverse &universe;
	const BarzelBead	&d_bead;
    const Barz& d_barz;
	size_t lvl;

	bool d_printTtok;
    const BarzStreamerXML& d_streamer;
public:
	BeadVisitor(std::ostream &s, const StoredUniverse &u, const BarzelBead& b, const Barz& barz, const BarzStreamerXML& streamer ) : 
		os(s), universe(u), d_bead(b),d_barz(barz), lvl(0), d_printTtok(true), d_streamer(streamer){}

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
                        xmlEscape(tokStr, sstrBody);
                    }

                    if( needOffsetLengthVec ) 
                        offsetLengthVec.push_back( ttok.getOrigOffsetAndLength() );
				}
			}
		}	

        os << "<srctok";
        if( needOffsetLengthVec ) {
            os << " origmarkup=\"" ;
            for( auto i = offsetLengthVec.begin(); i!= offsetLengthVec.end(); ++i ) {
                if(i!= offsetLengthVec.begin() )
                    os << ";";
                std::pair< size_t, size_t > pp = d_barz.getGlyphFromOffsets(i->first,i->second);
                os << pp.first << "," << pp.second;
            }
            os << "\" bytemarkup=\"" ;
            for( auto i = offsetLengthVec.begin(); i!= offsetLengthVec.end(); ++i ) {
                if(i!= offsetLengthVec.begin() )
                    os << ";";
                os << i->first << "," << i->second;
            }
            os << "\">";
        } else {
            os << ">";
        }
		os << sstrBody.str() << "</srctok>";
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
                        os << "<punct>";
                        xmlEscape(cstr, os);
                        os << "</punct>";
                    } else {
                        os << "<token>";
                        xmlEscape(cstr, os);
                        os << "</token>";
                    }
                } else {
                    // AYLOG(ERROR) << "Illegal literal ID: " << std::hex << data.getId();
                    os << "<null/>";
                }
			}
			break;
		case BarzerLiteral::T_STOP:
			{
				if (data.getId() == INVALID_STORED_ID) {
					os << "<fluff>";
					os << "</fluff>";
				} else {
					const char *cstr = universe.getStringPool().resolveId(data.getId());
					if (cstr) {
                        xmlEscape(cstr, os << "<fluff>");
                        os << "</fluff>";
					} 
					else AYLOG(ERROR) << "Illegal literal(STOP) ID: " << std::hex << data.getId();
				}
			}
			break;
		case BarzerLiteral::T_PUNCT:
			{ // need to somehow make this localised
				os << "<punct>";
				const char str[] = { (char)data.getId(), '\0' };
				xmlEscape(str, os);
				os << "</punct>";
			}
			break;
		case BarzerLiteral::T_BLANK:
			//os << "<blank />";
		    return false;
			break;
		default:
			AYLOG(ERROR) << "Unknown literal type";
			os << "<error>unknown literal type</error>";
		}
		return true;
	}
	bool operator()(const BarzerNone &data) {
		return false;
	}
	bool operator()(const BarzerString &data) {
	    tag_raii tok(os, ( data.isFluff() ? "fluff" : "token" ));
		//xmlEscape(data.getStr(), os << "<token>");
	    xmlEscape(data.getStr(), tok.os);
		//os << "</token>";
	    return true;
	}
	bool operator()(const BarzerNumber &data) {
		const char *type =  data.isReal() ? "real" : (data.isInt() ? "int" : "NaN");
		data.print(os << "<num t=\"" << type << "\">");
		os << "</num>";
		return true;
	}
	bool operator()(const BarzerDate &data) {
		//printTo(os << "<date>", data) << "</date>";
		// tag_raii td(os,"date");
        os << boost::format("<date y=\"%1%\" mon=\"%2%\" d=\"%3%\">") 
            % (int)data.getYear() 
            % (int)data.getMonth() 
            % (int)data.getDay() ;
		printTo(os,data);
        os << "</date>";
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

		void operator() ( const BarzerRange::Literal& d ) 
		{
            if( !d.first.isNull() ) {
			    bvis.os << "<lo>"; bvis.operator()( d.first ); bvis.os << "</lo>";
            }
            if( !d.second.isNull() ) {
			    bvis.os << "<hi>"; bvis.operator()( d.second ); bvis.os << "</hi>";
            }
        }
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

	void printEntity(const BarzerEntity &euid, const char* attrs = 0) {
		os << "<entity ";

        const char* tokname = universe.getGlobalPools().internalString_resolve(euid.tokId);
		if( tokname ) {
			if (tokname) {
				xmlEscape(tokname, os << "id=\"") << "\" ";
			}
		} else if( euid.isTokIdValid() ) {
			os << "INVALID_TOK[" << euid.eclass << "," << std::hex << euid.tokId << "]";
		}

        const EntityData::EntProp* edata = universe.getEntPropData( euid );
        if( edata ) { 
            if( edata->canonicName.length() ) 
                xmlEscape( edata->canonicName.c_str(), os << "n=\"" ) << "\" ";
            os << " r=\"" << edata->relevance << "\" " ;
        }

        if( d_barz.hasProperties() ) {
            if( attrs ) {
                static const char *tmpl = "class=\"%1%\" subclass=\"%2%\" %3%>";
                os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass % attrs;
            } else {
                static const char *tmpl = "class=\"%1%\" subclass=\"%2%\">";
                os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass;
            }
            const Ghettodb& gd = universe.getGhettodb();  
            PropCallback callback(os);
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
                    PropCallback zeroUniverseCallback(os,0);
                    zeroGd.iterateProperties(
                        zeroUniverseCallback,
                        d_barz.topicInfo.getPropNamesZeroUniverse().begin(), 
                        d_barz.topicInfo.getPropNamesZeroUniverse().end(), 
                        euid );
                }
            }

		    os << "</entity>";
        } else {
            if( attrs ) {
                static const char *tmpl = "class=\"%1%\" subclass=\"%2%\" %3% />";
                os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass % attrs;
            } else {
                static const char *tmpl = "class=\"%1%\" subclass=\"%2%\" />";
                os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass;
            }
        } 
	}



	// not sure how to properly deconstruct this yet
	bool operator()(const BarzerEntityList &data) {
        if( data.getList().size() == 1 ) {
            printEntity(data.getList()[0]);
        } else {
		    //os << "<entlist>";
            std::stringstream sstr;
            if( data.getClass().isValid() ) {
                sstr << " class=\"" << data.getClass().ec << "\" subclass=\"" << data.getClass().subclass << "\"";
            }
         
	        tag_raii el(os, "entlist", sstr.str().c_str());

		    const BarzerEntityList::EList &lst = data.getList();
		    for (BarzerEntityList::EList::const_iterator li = lst.begin();
													    li != lst.end(); ++li) {
                os << "\n    ";
			    printEntity(*li);
		    }
		    os << "\n    ";
        }
		return true;
	}

	bool operator()(const BarzerEntity &data) {
		printEntity(data);
		return true;
	}

	bool operator()(const BarzerEVR &data) {
		tag_raii erctag(os, "evr");
        {
		    (*this)(data.getEntity());
            for( auto i = data.data().begin(), i_end= data.data().end(); i!= i_end; ++i ) {
                std::string attr = i->first;
                if( i->first.length() ) {
                    std::stringstream sstr; 
                    sstr << "n=\"";
                    xmlEscape( i->first.c_str(), sstr ) << "\"";
                    attr = sstr.str();
                }
                tag_raii erctag(os, "variant", attr.c_str());
                for( auto j =i->second.begin(), j_end = i->second.end(); j!= j_end; ++j ) {
                    boost::apply_visitor( (*this), *j );
                }
            }
        }
        return true;
    }
	bool operator()(const BarzerERC &data) {
		const StoredEntityUniqId &ent = data.getEntity(),
						         &unit = data.getUnitEntity();

		tag_raii erctag(os, "erc");
		{ /// block 
		(*this)(ent);
		if (unit.isValid()) {
			//os << "<unit>";
		    tag_raii unittag(os, "unit");
			(*this)(unit);
			//os << "</unit>";
		}
		(*this)(data.getRange());
		} // end of block
		//os << "</erc>";
		return true;
	}
	bool operator()(const BarzerERCExpr &exp) {
		os << "<ercexpr type=\"" << exp.getTypeName() << "\">";
		{ // the block
		ay::valkeep<bool> vk( d_printTtok, false );
		const BarzerERCExpr::DataList &list = exp.getData();
		for (BarzerERCExpr::DataList::const_iterator it = list.begin();
													 it != list.end(); ++it) {
			boost::apply_visitor(*this, *it);
		}
		} // end of block 
		os << "</ercexpr>";
		return true;
	}
	bool operator()(const BarzelBeadBlank&)
	    { return false; }
	bool operator()(const BarzelBeadAtomic &data)
	{
		//AYLOG(DEBUG) << "atomic: " << data.getType();
		os << "    ";
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
		tag_raii tr(os);
		if (!(lvl++)) tr.push("expr");
		const char *tagname = universe.getStringPool().resolveId(data.sid);
		if (tagname) {
			os << "<" << tagname;
			printAttributes(data.getAttrs());
			const BBE::SubExprList &selst = data.getChildren();
			if (selst.size()) {
				os << ">";
				for (BBE::SubExprList::const_iterator it = selst.begin();
													  it != selst.end(); ++it) {
					boost::apply_visitor(*this, *it);
				}
				os << "</" << tagname << ">";
			} else os << " />";
		} else AYLOG(ERROR) << "Unknown string id: " << data.sid;
		return true;
		//os << "</expression>";
	}
	void clear() {
		lvl = 0;
	}
};
}

static void printTraceInfo(std::ostream &os, const Barz &barz, const StoredUniverse &uni)
{
    static const char *tmpl = "<match gram=\"%4%\" file=\"%1%\" stmt=\"%2%\" emit=\"%3%\"";

    const BarzelTrace::TraceVec &tvec = barz.barzelTrace.getTraceVec();
    const GlobalPools &gp = uni.getGlobalPools();
    if( tvec.size() ) {
        tag_raii ti(os, "traceinfo");
        os << "\n";
        BarzelTranslationTraceInfo::Vec btiVec;
        for( BarzelTrace::TraceVec::const_iterator ti = tvec.begin(), tend = tvec.end(); ti != tend; ++ti ) {
            const char *name = gp.internalString_resolve( ti->tranInfo.source );
            if( !name ) name ="";

            const BELTrie* trie = gp.getTriePool().getTrie_byGlobalId(ti->globalTriePoolId) ;
            if( trie ) {
                btiVec.clear();
                if( trie->getLinkedTraceInfo(btiVec,ti->tranId) && !btiVec.empty() ) {
                    os << "<match gram=\"" << ti->grammarSeqNo << "\" " ;
                    xmlEscape( name, os << "file=\"" ) << "\" " <<
                    "stmt=\"" << btiVec[0].first.statementNum  << "\" " <<
                    "emit=\"" << btiVec[0].first.emitterSeqNo  << "\">";

                    for( size_t j = 1; j< btiVec.size(); ++j ) {
                        const auto& x = btiVec[j];
                        const auto& i = x.first;
                        if( !(ti->tranInfo.statementNum== i.statementNum && i.source== ti->tranInfo.source ) ) {
                            const char *linkedName = gp.internalString_resolve_safe( i.source );
                            xmlEscape( linkedName, os << "\n  <linkedmatch file=\"" ) << 
                                "\" stmt=\"" << i.statementNum << "\" emit=\"" << i.emitterSeqNo << "\"/>";
                        }
                    }
                } else {
                    os << boost::format(tmpl) % (name ? name : "")
                                      % ti->tranInfo.statementNum
                                      % ti->tranInfo.emitterSeqNo
                                      % ti->grammarSeqNo;
                    if( ti->errVec.empty() )
                        os << "/>";
                    else 
                        os << ">";
                }
            }
            if( ti->errVec.size()) {
                os << "\n";
                os << " <error>";
                for( std::vector< std::string >::const_iterator ei = ti->errVec.begin(); ei!= ti->errVec.end(); ++ei ) {
                    os << *ei << " / ";
                }

                os << " </error></match>\n";
            } else if( !btiVec.empty() ) {
                os << "\n</match>";
            }
            os << "\n";
        }
    }

}

namespace {

inline bool stringPair_comp_less( const CToken::StringPair& l, const CToken::StringPair& r ) 
{ return( l.first < r.first ); }
inline bool stringPair_comp_eq( const CToken::StringPair& l, const CToken::StringPair& r ) 
{ return( l.first == r.first ); }

}

namespace {

void print_conf_leftovers( std::ostream& os, const std::vector<std::string>& vec, const char* attr  ) 
{
        os << "<leftover t=\"" << attr << "\">\n";
        std::set< std::string > tmp;
        for( const auto& i : vec ) {
            if( tmp.find(i) == tmp.end() ) {
                xmlEscape( i, (os << "    <text s=\"")) << "\"/>\n";
                tmp.insert(i);
            }
        }
        os << "</leftover>\n" ;
}

}

std::ostream& BarzStreamerXML::printConfidence(std::ostream &os)
{
    const BarzConfidenceData& confidenceData = barz.confidenceData; 
    if( !confidenceData.hasAnyConfidence() ) 
        return os;

    os << "<confidence>\n";
    if( confidenceData.d_loCnt ) {
        std::vector< std::string > tmp ;
        confidenceData.fillString( tmp, barz.getOrigQuestion(), BarzelBead::CONFIDENCE_LOW );
        if( tmp.size() )
            print_conf_leftovers( os, tmp, "nolo" );
    }
    if( confidenceData.d_medCnt ) {
        std::vector< std::string > tmp ;
        confidenceData.fillString( tmp, barz.getOrigQuestion(), BarzelBead::CONFIDENCE_MEDIUM );
        if( tmp.size() )
            print_conf_leftovers( os, tmp, "nomed" );
    }
    if( confidenceData.d_hiCnt ) {
        std::vector< std::string > tmp ;
        confidenceData.fillString( tmp, barz.getOrigQuestion(), BarzelBead::CONFIDENCE_HIGH );
        if( tmp.size() )
            print_conf_leftovers( os, tmp, "nohi" );
    }
    os << "</confidence>\n";
    return os;
}
std::ostream& BarzStreamerXML::print(std::ostream &os)
{
    /// BARZ header tag 
	os << "<barz" << " u=\"" << std::dec << universe.getUserId() << "\"" ;
    if( barz.isQueryIdValid() ) 
        os << " qid=\"" << std::dec << barz.getQueryId() << "\"";
    os << ">";
    /// end of BARZ header tag
    
	const BarzelBeadChain &bc = barz.getBeads();
	CToken::SpellCorrections spellCorrections;
	size_t curBeadNum = 1;
	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
	    if (!isBlank(*bli)) {
	        os << "\n";
	        // tag_raii beadtag(os, "bead");
			os << "<bead n=\"" << curBeadNum << "\"";
            if( bli->isComplexAtomicType() && bli->getConfidence()!= BarzelBead::CONFIDENCE_UNKNOWN ) 
                os << " c=\"" << bli->getConfidence() << "\"";
            os << ">\n" ;
	        BeadVisitor v(os, universe, *bli, barz, *this );
	        if (boost::apply_visitor(v, bli->getBeadData())) {
	            os << "\n    ";
	            v.printTTokenTag();
	        }
	        os << "\n</bead>\n";
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
    /// printing topics 
    const BarzTopics::TopicMap& topicMap = barz.topicInfo.getTopicMap();
    if( !topicMap.empty() ) {
        os << "<topics>\n";
        BarzelBead fakeBead;
	    BeadVisitor v(os, universe, fakeBead, barz, *this  );
        for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
            os << "    ";
            std::stringstream sstr;
            sstr << "strength=\"" << topI->second << "\"";
            v.printEntity( topI->first, sstr.str().c_str() );
        }
        os << "\n</topics>\n";
    }

    if( !barz.d_beni.empty() ) {
	    tag_raii tok(os, "beni");
        BarzelBead fakeBead;
	    BeadVisitor v(os, universe, fakeBead, barz, *this  );
        for( const auto&i : barz.d_beni.d_entVec ) {
            std::stringstream sstr;
            sstr << "rank=\"" << i.popRank << "\" ";
			sstr << "cov=\"" << i.coverage << "\" ";
			sstr << "rel=\"" << i.relevance << "\"";
            v.printEntity( i.ent, sstr.str().c_str() );
            os << std::endl;
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
        xmlEscape( barz.getOrigQuestion().c_str(),  os) << "</query>\n";
    }

    /// confidence
    if( universe.checkBit(UBIT_NEED_CONFIDENCE) ) 
        printConfidence(os);

	os << "</barz>\n";
	return os;
}

namespace {
std::ostream& jsonEscape(const char* tokname, std::ostream& os )
{
    for( const char* s = tokname; *s; ++s ) {
        switch( *s ) {
        case '\\': os << "\\\\"; break;
        case '"': os << "\\\""; break;
        default: os << *s; break;
        }
    }
    return os;
}

}
std::ostream& AutocStreamerJSON::print(std::ostream &os) const
{
    const BestEntities::EntWeightMap& entWMap = bestEnt.getEntitiesAndWeights();
    os << "{\"data\":[";
    const GlobalPools& gp = universe.getGlobalPools();
    for( BestEntities::EntWeightMap::const_iterator i = entWMap.begin(); i!= entWMap.end(); ++i ) {
        const BarzerEntity& euid = i->second;
        const BestEntities_EntWeight& eweight = i->first;
		// const StoredToken *tok = universe.getDtaIdx().tokPool.getTokByIdSafe(euid.tokId);
	    const char *tokname = universe.getGlobalPools().internalString_resolve(euid.tokId);
		if( tokname ) {
            os<< ( i != entWMap.begin() ? ",":"" ) << "\n{";
			jsonEscape(tokname, os << "\"id\":\"") << "\"";
            const EntityData::EntProp* edata = universe.getEntPropData( euid );
            uint32_t eclass = euid.eclass.ec, esubclass = euid.eclass.subclass;
            os << ",\"cl\":\"" << std::dec << eclass<< "\"," << "\"sc\":\"" << esubclass << 
            "\",\"ord\":\"" << eweight.pathLen << "." << eweight.relevance << "\"";
            if( edata ) {
                os << ",\"n\":\"" << edata->canonicName.c_str() << "\"";
            }
            os << "}";
		}
        
    }
    os << "\n]}\n";
    return os;
}

}
