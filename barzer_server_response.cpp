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
#include <sstream>

namespace barzer {

namespace {

struct tag_raii {
	std::ostream &os;
	std::vector<const char*> tags;

	tag_raii(std::ostream &s) : os(s) {}
	tag_raii(std::ostream &s, const char *tag, const char* attr = 0) : os(s) 
        { push(tag,attr); }
	operator std::ostream&() { return os; }

	void push(const char *tag, const char* attr=0 ) {
		os << "<" << tag << ( attr ? attr : "" ) << ">";
		tags.push_back(tag);
	}

	~tag_raii() {
		size_t i = tags.size();
		while(i--) {
			os << "</" << tags.back() << ">";
			tags.pop_back();
		}
	}
};

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

// need to find an xml library for this kind of stuff
static std::ostream& xmlEscape(const char *src,  std::ostream &os) {
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

static inline std::ostream& xmlEscape(const std::string &src, std::ostream &os) {
	return xmlEscape(src.c_str(), os);
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

class BeadVisitor : public boost::static_visitor<bool> {
	std::ostream &os;
	const StoredUniverse &universe;
	const BarzelBead	&d_bead;
	size_t lvl;

	bool d_ptintTtok;
public:
	BeadVisitor(std::ostream &s, const StoredUniverse &u, const BarzelBead& b) : 
		os(s), universe(u), d_bead(b),  lvl(0), d_ptintTtok(true) {}

	void printTTokenTag( )
	{
		if( !d_ptintTtok ) return;

		const CTWPVec& ctoks = d_bead.getCTokens();
		os << "<srctok>";
		for( CTWPVec::const_iterator ci = ctoks.begin(); ci != ctoks.end(); ++ci ) {
			const TTWPVec& ttv = ci->first.getTTokens();
			for( TTWPVec::const_iterator ti = ttv.begin(); ti!= ttv.end() ; ++ti ) {
				const TToken& ttok = ti->first;
				if( ttok.len && ttok.buf ) {
					os.write( ttok.buf, ttok.len ) << " ";
				}
			}
		}	
		os << "</srctok>";
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
                    AYLOG(ERROR) << "Illegal literal ID: " << std::hex << data.getId();
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
                        if( !ispunct(*cstr) ) {
                            xmlEscape(cstr, os << "<fluff>");
                            os << "</fluff>";
                        } else 
                            os << "<fluff/>";
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
	    tag_raii tok(os, "token");
		//xmlEscape(data.getStr(), os << "<token>");
	    xmlEscape(data.getStr(), tok);
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
		tag_raii td(os,"date");
		printTo(os,data);
		return true;
	}
	bool operator()(const BarzerTimeOfDay &data) {

		tag_raii td(os, "time");
		printTo(os, data);
		return true;
	}
	bool operator()(const BarzerDateTime &data) {
		tag_raii td(os, "timestamp");
		printTo(os, data);
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

		os << "<range order=\"" << (data.isAsc() ? "ASC" : "DESC") <<  "\"";
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
		const StoredToken *tok = universe.getDtaIdx().tokPool.getTokByIdSafe(euid.tokId);
		if( tok ) {
			const char *tokname = universe.getStringPool().resolveId(tok->stringId);
			if (tokname) {
				xmlEscape(tokname, os << "id=\"") << "\" ";
			}
		} else if( euid.isTokIdValid() ) {
			os << "INVALID_TOK[" << euid.eclass << "," << std::hex << euid.tokId << "]";
		}
        if( attrs ) {
            static const char *tmpl = "class=\"%1%\" subclass=\"%2%\" %3% />";
            os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass % attrs;
        } else {
            static const char *tmpl = "class=\"%1%\" subclass=\"%2%\" />";
            os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass;
        }
		//os << "</entity>";
	}



	// not sure how to properly deconstruct this yet
	bool operator()(const BarzerEntityList &data) {
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
		return true;
	}

	bool operator()(const BarzerEntity &data) {
		printEntity(data);
		return true;
	}

	bool operator()(const BarzerEntityRangeCombo &data) {
		const StoredEntityUniqId &ent = data.getEntity(),
						         &unit = data.getUnitEntity();

		//os << "<erc>";
		tag_raii erctag(os, "erc");
		{ /// block 
		//ay::valkeep<bool> vk( d_ptintTtok, false );
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
		ay::valkeep<bool> vk( d_ptintTtok, false );
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
        for( BarzelTrace::TraceVec::const_iterator ti = tvec.begin(),
                                                  tend = tvec.end();
                    ti != tend; ++ti ) {
            const char *name = gp.internalString_resolve( ti->tranInfo.source );
            os << boost::format(tmpl) % (name ? name : "")
                                      % ti->tranInfo.statementNum
                                      % ti->tranInfo.emitterSeqNo
                                      % ti->grammarSeqNo;
            if( ti->errVec.size()) {
                os << ">";
                os << " <error>";
                for( std::vector< std::string >::const_iterator ei = ti->errVec.begin(); ei!= ti->errVec.end(); ++ei ) {
                    os << *ei << " ";
                }
                os << " </error></match>\n";
            } else {
                os << "/>";
            }
            os << "\n";
        }
    }

}

std::ostream& BarzStreamerXML::print(std::ostream &os)
{
	os << "<barz>";
	const BarzelBeadChain &bc = barz.getBeads();
	CToken::SpellCorrections spellCorrections;
	size_t curBeadNum = 1;
	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
	    if (!isBlank(*bli)) {
	        os << "\n";
	        // tag_raii beadtag(os, "bead");
			os << "<bead n=\"" << curBeadNum << "\">\n" ;
	        BeadVisitor v(os, universe, *bli);
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
	    BeadVisitor v(os, universe, fakeBead );
        for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
            os << "    ";
            std::stringstream sstr;
            sstr << "strength=\"" << topI->second << "\"";
            v.printEntity( topI->first, sstr.str().c_str() );
        }
        os << "\n</topics>\n";
    }


	/// printing spell corrections  if any 
	if( spellCorrections.size( ) ) {
		os << "<spell>\n";
		for( CToken::SpellCorrections::const_iterator i = spellCorrections.begin(); i!= spellCorrections.end(); ++i ) {
			os << "<correction before=\"" << i->first << "\" after=\"" << i->second << "\"/>\n";
		}
		os << "</spell>\n";
	}
    printTraceInfo(os, barz, universe);
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
    os << "{data:[";
    for( BestEntities::EntWeightMap::const_iterator i = entWMap.begin(); i!= entWMap.end(); ++i ) {
        const BarzerEntity& euid = i->second;
        const BestEntities_EntWeight& eweight = i->first;
		const StoredToken *tok = universe.getDtaIdx().tokPool.getTokByIdSafe(euid.tokId);
		if( tok ) {
            os<< ( i != entWMap.begin() ? ",":"" ) << "\n{";
			const char *tokname = universe.getStringPool().resolveId(tok->stringId);
			if (tokname) {
				jsonEscape(tokname, os << "id:\"") << "\"";
			}
            uint32_t eclass = euid.eclass.ec, esubclass = euid.eclass.subclass;
            os << ",cl:\"" << std::dec << eclass<< "\"," << "sc:\"" << esubclass << 
            "\",ord:\"" << eweight.pathLen << "." << eweight.relevance << "\"}";
		}
        
    }
    os << "\n]}\n";
    return os;
}

}
