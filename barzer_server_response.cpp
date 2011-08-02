/*
 * barzer_server_response.cpp
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#include <barzer_server_response.h>
#include <ay/ay_logger.h>
#include <ay/ay_raii.h>
#include <sstream>
#include <boost/format.hpp>
#include <barzer_universe.h>

namespace barzer {

namespace {

struct tag_raii {
	std::ostream &os;
	std::vector<const char*> tags;

	tag_raii(std::ostream &s) : os(s) {}
	tag_raii(std::ostream &s, const char *tag) : os(s) { push(tag); }
	operator std::ostream&() { return os; }

	void push(const char *tag) {
		os << "<" << tag << ">";
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


class RangeVisitor : public boost::static_visitor<> {
	std::ostream &os;
public:
	RangeVisitor(std::ostream &s) : os(s) {}

	template<class T> std::ostream& lohi(const T lo, const T hi) {
		os << std::dec
		   << "<lo>" << lo << "</lo>"
		   << "<hi>" << hi << "</hi>";
		return os;
	}

	template<class T> std::ostream& lohi(const std::pair<T,T> &p) {
		printTo(os << "<lo>", p.first) << "</lo>";
		printTo(os << "<hi>", p.second) << "</hi>";
		return os;
	}

	void operator()(const BarzerRange::None &data) {
		os << "<norange/>";
	}
	void operator()(const BarzerRange::Integer &data) {
		os << "<num t=\"int\">";
		lohi(data.first, data.second) << "</num>";
	}
	void operator()(const BarzerRange::Real &data) {
		os << "<num t=\"real\">";
		lohi(data.first, data.second) << "</num>";
	}
	void operator()(const BarzerRange::TimeOfDay &data) {
		tag_raii t(os, "time");
		lohi(data);
		//os << "</time>";
	}
	void operator()(const BarzerRange::Date &data) {
		//os << "<date>";
	    tag_raii d(os, "date");
		lohi(data);
		//os << "</date>";
	}
	void operator()(const BarzerRange::DateTime &data) {
		//os << "<timestamp>";
	    tag_raii ts(os, "timestamp");
		lohi(data);
		//os << "</timestamp>";
	}
	void operator()(const BarzerRange::Entity &data) {
		os << "<entrange>";

		//printEntity(const BarzerEntity &euid)
		os << "</entrange>";
	}

};

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
					os.write( ttok.buf, ttok.len );
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
				os << "<token>";
				const char *cstr = universe.getStringPool().resolveId(data.getId());
				if (cstr) xmlEscape(cstr, os);
				else AYLOG(ERROR) << "Illegal literal ID: " << std::hex << data.getId();
				//printTTokenTag();
				os << "</token>";
			}
			break;
		case BarzerLiteral::T_STOP:
			{
				if (data.getId() == INVALID_STORED_ID) {
					os << "<fluff>";
					//printTTokenTag();
					os << "</fluff>";
				} else {
					const char *cstr = universe.getStringPool().resolveId(data.getId());
					if (cstr) {
						xmlEscape(cstr, os << "<fluff>");
						//printTTokenTag();
						os << "</fluff>";
					} 
					else AYLOG(ERROR) << "Illegal literal(STOP) ID: " << std::hex << data.getId();
				}
			}
			break;
		case BarzerLiteral::T_PUNCT:
			{ // need to somehow make this localised
				os << "<token>";
				const char str[] = { (char)data.getId(), '\0' };
				xmlEscape(str, os);
				os << "</token>";
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
	bool operator()(const BarzerString &data) {
	    tag_raii tok(os, "token");
		//xmlEscape(data.getStr(), os << "<token>");
	    xmlEscape(data.getStr(), tok);
		//printTTokenTag();
		//os << "</token>";
	    return true;
	}
	bool operator()(const BarzerNumber &data) {
		const char *type =  data.isReal() ? "real" : (data.isInt() ? "int" : "NaN");
		data.print(os << "<num t=\"" << type << "\">");
		//printTTokenTag();
		os << "</num>";
		return true;
	}
	bool operator()(const BarzerDate &data) {
		//printTo(os << "<date>", data) << "</date>";
		tag_raii td(os,"date");
		printTo(os,data);
		//printTTokenTag();
		return true;
	}
	bool operator()(const BarzerTimeOfDay &data) {

		tag_raii td(os, "time");
		printTo(os, data);
		//printTTokenTag();
		return true;
	}
	bool operator()(const BarzerDateTime &data) {
		tag_raii td(os, "timestamp");
		printTo(tag_raii(os, "timestamp"), data);
		//printTTokenTag();
		return true;
	}
	bool operator()(const BarzerRange &data) {

		os << "<range order=\"" << (data.isAsc() ? "ASC" : "DESC") <<  "\"";
		if (data.isBlank()) os << " />";
		else {
			os << ">";
			RangeVisitor v(os);
			{ /// 
			//ay::valkeep<bool> vk( d_ptintTtok, false );
			boost::apply_visitor(v, data.dta);
			}
			//printTTokenTag();
			os << "</range>";
		}
		return true;
	}

	void printEntity(const BarzerEntity &euid) {
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
		static const char *tmpl = "class=\"%1%\" subclass=\"%2%\" />";
		os << boost::format(tmpl) % euid.eclass.ec % euid.eclass.subclass;
		//printTTokenTag();
		//os << "</entity>";
	}



	// not sure how to properly deconstruct this yet
	bool operator()(const BarzerEntityList &data) {
		//os << "<entlist>";
	    tag_raii el(os, "entlist");
		const BarzerEntityList::EList &lst = data.getList();
		for (BarzerEntityList::EList::const_iterator li = lst.begin();
													 li != lst.end(); ++li) {
			printEntity(*li);
		}
		//printTTokenTag();
		//os << "</entlist>";
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
		//printTTokenTag();
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
		//printTTokenTag();
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
				//printTTokenTag();
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



std::ostream& BarzStreamerXML::print(std::ostream &os)
{
	os << "<barz>";
	const BarzelBeadChain &bc = barz.getBeads();
	CToken::SpellCorrections spellCorrections;

	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
	    if (!isBlank(*bli)) {
	        os << "\n";
	        tag_raii beadtag(os, "bead");
	        os << "\n";
	        BeadVisitor v(os, universe, *bli);
	        if (boost::apply_visitor(v, bli->getBeadData())) {
	            os << "\n    ";
	            v.printTTokenTag();
	        }
	        os << "\n";
	    }


		//// accumulating spell corrections in spellCorrections vector 
		const CTWPVec& ctoks = bli->getCTokens();
		for( CTWPVec::const_iterator ci = ctoks.begin(); ci != ctoks.end(); ++ci ) {
			const CToken::SpellCorrections& corr = ci->first.getSpellCorrections(); 
			spellCorrections.insert( spellCorrections.end(), corr.begin(), corr.end() );
		}
		/// end of spell corrections accumulation
	}
	/// printing spell corrections  if any 
	if( spellCorrections.size( ) ) {
		os << "<spell>\n";
		for( CToken::SpellCorrections::const_iterator i = spellCorrections.begin(); i!= spellCorrections.end(); ++i ) {
			os << "<correction before=\"" << i->first << "\" after=\"" << i->second << "\"/>\n";
		}
		os << "</spell>\n";
	}
	os << "</barz>\n";
	return os;
}

}
