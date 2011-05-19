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
		//os << "<norange/>";
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

};

class AtomicVisitor : public boost::static_visitor<> {
	std::ostream &os;
	StoredUniverse &universe;
public:
	AtomicVisitor(std::ostream &s, StoredUniverse &u) : os(s), universe(u) {}

	void operator()(const BarzerLiteral &data) {
		if (data.isBlank()) return;
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
			os << "<blank />";
			break;
		default:
			AYLOG(ERROR) << "Unknown literal type";
			os << "<error>unknown literal type</error>";
		}
	}
	void operator()(const BarzerString &data) {
		os << "<token>";
		xmlEscape(data.getStr(), os) << "</token>";
	}
	void operator()(const BarzerNumber &data) {
		const char *type =  data.isReal() ? "real" : (data.isInt() ? "int" : "NaN");
		data.print(os << "<num t=\"" << type << "\">") << "</num>";
	}
	void operator()(const BarzerDate &data) {
		printTo(os << "<date>", data) << "</date>";
	}
	void operator()(const BarzerTimeOfDay &data) {

		printTo(os << "<time>", data) << "</time>";
	}
	void operator()(const BarzerDateTime &data) {
		os << "<timestamp>";
		(*this)( data.getDate() );
		(*this)( data.getTime() );
		os << "</timestamp>";
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

	void printEntity(const StoredEntity &ent) {
		const StoredEntityUniqId &euid = ent.euid;
		const StoredToken &tok = universe.getDtaIdx().tokPool.getTokById(euid.tokId);
		const char *tokname = universe.getStringPool().resolveId(tok.stringId);
		os << "<entity"
				<< " id=\"" << tokname << "\""
		        << " class=\"" << euid.eclass.ec << "\""
				<< " subclass=\"" << euid.eclass.subclass << "\""
				<< " />";
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
			os << boost::format("<entity cl=\"%1%\" scl=\"%2%\" />")
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
		printEntity(*ent);
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
};
}

std::ostream& BarzStreamerXML::print(std::ostream &os)
{
	os << "<barz>\n";
	const BarzelBeadChain &bc = barz.getBeads();
	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
		const BarzelBead &bead = *bli;
		if (bead.isBlank()) {
			os << " ";
		} else if (bead.isAtomic()) {
			os << "    ";
			AtomicVisitor v(os, universe);
			boost::apply_visitor(v, bead.getAtomic()->dta);
			os << "\n";
		} else if (bead.isExpression()) {
			// not quite sure what to do with it yet
			// const BarzelBeadExpression *bbe = bead.getExpression();
			os << "<expression>";
			os << "</expression>";
		} else {
			AYLOG(ERROR) << "Unknown bead type";
			// then wtf is this
		}
	}
	os << "</barz>\n";
	return os;
}

}
