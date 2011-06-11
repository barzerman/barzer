/*
 * barzer_server_response.cpp
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#include <barzer_server_response.h>
#include <ay/ay_logger.h>
#include <sstream>
#include <boost/format.hpp>

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
		os << "<lo>" << lo << "</lo>";
		os << "<hi>" << hi << "</hi>";
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
		os << "<time>";
		lohi(data);
		os << "</time>";
	}
	void operator()(const BarzerRange::Date &data) {
		os << "<date>";
		lohi(data);
		os << "</date>";
	}
	void operator()(const BarzerRange::DateTime &data) {
		os << "<timestamp>";
		lohi(data);
		os << "</timestamp>";
	}
	void operator()(const BarzerRange::Entity &data) {
		os << "<entrange>";

		//printEntity(const BarzerEntity &euid)
		os << "</entrange>";
	}

};

class BeadVisitor : public boost::static_visitor<> {
	std::ostream &os;
	StoredUniverse &universe;
	size_t lvl;
	
public:
	BeadVisitor(std::ostream &s, StoredUniverse &u) : os(s), universe(u), lvl(0) {}

	void operator()(const BarzerLiteral &data) {
		//AYLOG(DEBUG) << "BarzerLiteral";
		switch(data.getType()) {
		case BarzerLiteral::T_STRING:
		case BarzerLiteral::T_COMPOUND:
			{
				os << "<token>";
				const char *cstr = universe.getStringPool().resolveId(data.getId());
				if (cstr) xmlEscape(cstr, os);
				else AYLOG(ERROR) << "Illegal literal ID: " << std::hex << data.getId();
				os << "</token>";
			}
			break;
		case BarzerLiteral::T_STOP:
			{
				if (data.getId() == 0xffffffff) {
					os << "<fluff />";
				} else {
					const char *cstr = universe.getStringPool().resolveId(data.getId());
					if (cstr) xmlEscape(cstr, os << "<fluff>") << "</fluff>";
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
			break;
		default:
			AYLOG(ERROR) << "Unknown literal type";
			os << "<error>unknown literal type</error>";
		}
	}
	void operator()(const BarzerString &data) {
		xmlEscape(data.getStr(), os << "<token>") << "</token>";
	}
	void operator()(const BarzerNumber &data) {
		const char *type =  data.isReal() ? "real" : (data.isInt() ? "int" : "NaN");
		data.print(os << "<num t=\"" << type << "\">") << "</num>";
	}
	void operator()(const BarzerDate &data) {
		//printTo(os << "<date>", data) << "</date>";
		printTo(tag_raii(os,"date"), data);
	}
	void operator()(const BarzerTimeOfDay &data) {

		//printTo(os << "<time>", data) << "</time>";
		printTo(tag_raii(os, "time"), data);
	}
	void operator()(const BarzerDateTime &data) {
		printTo(tag_raii(os, "timestamp"), data);
				/*
		os << "<timestamp>";
		//(*this)( data.getDate() );
		//(*this)( data.getTime() );

		os << "</timestamp>";
		*/
	}
	void operator()(const BarzerRange &data) {

		os << "<range order=\"" << (data.isAsc() ? "ASC" : "DESC") <<  "\"";
		if (data.isBlank()) os << " />";
		else {
			os << ">";
			RangeVisitor v(os);
			boost::apply_visitor(v, data.dta);
			os << "</range>";
		}

	}

	void printEntity(const BarzerEntity &euid) {
		const StoredToken *tok = universe.getDtaIdx().tokPool.getTokByIdSafe(euid.tokId);
		if( tok ) {
			const char *tokname = universe.getStringPool().resolveId(tok->stringId);
			static const char *tmpl =
					"<entity id=\"%1%\" class=\"%2%\" subclass=\"%3%\" />";
			os << boost::format(tmpl) % (tokname ? tokname : "(null)")
									  % euid.eclass.ec
									  % euid.eclass.subclass;
		} else {
			os << "INVALID_TOK[" << euid.eclass << "," << std::hex << euid.tokId << "]";
		}
		//os << "</entity>";
	}



	// not sure how to properly deconstruct this yet
	void operator()(const BarzerEntityList &data) {
		os << "<entlist>";
		const BarzerEntityList::EList &lst = data.getList();
		for (BarzerEntityList::EList::const_iterator li = lst.begin();
													 li != lst.end(); ++li) {
			printEntity(*li);
		}
		os << "</entlist>";
	}

	void operator()(const BarzerEntity &data) {
		if (data.tokId == 0xffffff) {
			os << boost::format("<entity class=\"%1%\" subclass=\"%2%\" />")
				% data.eclass.ec
				% data.eclass.subclass;
			return;
		}

		const StoredEntity *ent = universe.getDtaIdx().entPool.getEntByEuid(data);
		if (!ent) {
			AYLOG(ERROR) << "Invalid entity id: " << data.tokId << ","
												  << data.eclass.ec << ","
												  << data.eclass.subclass;
			return;
		}
		printEntity(ent->getEuid());
	}

	void operator()(const BarzerEntityRangeCombo &data) {
		const StoredEntityUniqId &ent = data.getEntity(),
						         &unit = data.getUnitEntity();

		os << "<erc>";
		(*this)(ent);
		if (unit.isValid()) {
			os << "<unit>";
			(*this)(unit);
			os << "</unit>";
		}
		(*this)(data.getRange());
		os << "</erc>";
	}
	void operator()(const BarzerERCExpr &exp) {
		os << "<ercexpr type=\"" << exp.getTypeName() << "\">";
		const BarzerERCExpr::DataList &list = exp.getData();
		for (BarzerERCExpr::DataList::const_iterator it = list.begin();
													 it != list.end(); ++it) {
			boost::apply_visitor(*this, *it);
		}

		os << "</ercexpr>";
	}
	void operator()(const BarzelBeadBlank&) {}
	void operator()(const BarzelBeadAtomic &data)
	{
		//AYLOG(DEBUG) << "atomic: " << data.getType();
		os << "    ";
		boost::apply_visitor(*this, data.getData());
		os << "\n";
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

	void operator()(const BarzelBeadExpression &data)
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
		//os << "</expression>";
	}
	void clear() {
		lvl = 0;
	}
};
}

std::ostream& BarzStreamerXML::print(std::ostream &os)
{
	os << "<barz>\n";
	const BarzelBeadChain &bc = barz.getBeads();
	BeadVisitor v(os, universe);
	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
		boost::apply_visitor(v, bli->getBeadData());
		v.clear();
	}
	os << "</barz>\n";
	return os;
}

}
