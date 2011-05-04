/*
 * barzer_server_response.cpp
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#include <barzer_server_response.h>
#include <ay/ay_logger.h>
#include <sstream>

namespace barzer {

namespace {


// need to find an xml library for this kind of stuff
static std::ostream& xmlEscape(const std::string &src, std::ostream &os) {
	//std::ostringstream ret;
	for(std::string::const_iterator i = src.begin(); i != src.end(); ++i) {
		char c = (char)*i;
		switch(c) {
		case '&': os << "&amp;"; break;
		case '<': os << "&lt;"; break;
		case '>': os << "&gt;"; break;
		case '"': os << "&quot;"; break;
		case '\'': os << "&apos;"; break;
		default: os << c;
		}
	}
	//return ret.str();
	return os;
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
		p.first.print(os << "<lo>") << "</lo>";
		p.second.print(os << "<hi>") << "</hi>";
		return os;
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
		case BarzerLiteral::T_COMPOUND: {
			os << "<token>";
			std::string s = universe.getStringPool().resolveId(data.getId());
			xmlEscape(s, os);
			os << "<token>";
		}
			break;
		case BarzerLiteral::T_STOP: {
			std::string s = universe.getStringPool().resolveId(data.getId());
			xmlEscape(s, os << "<fluff>") << "</fluff>";
		}
			break;
		case BarzerLiteral::T_PUNCT:
			{ // cough. this is ugly. also need to somehow make this localised
				os << "<token>";
				std::string s(1, (char)data.getId());
				xmlEscape(s, os);
				os << "<token>";
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
		os << "<date>";
		data.print(os) << "</date>";
	}
	void operator()(const BarzerTimeOfDay &data) {
		os << "<time>";
		data.print(os) << "</time>";
	}
	void operator()(const BarzerRange &data) {
		os << "<range>";
		RangeVisitor v(os);
		boost::apply_visitor(v, data.dta);
		os << "</range>";
	}

	void printEntity(const StoredEntity &ent) {
		os << "\n    <entity id=\"" << ent.entId << "\">";
		const StoredEntityUniqId &euid = ent.euid;
		os << "<euid"
				<< " tokid=\"" << euid.tokId << "\""
		        << " class=\"" << euid.eclass.ec << "\""
				<< " subclass=\"" << euid.eclass.subclass << "\""
				<< " />";
		os << "</entity>";
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

	void operator()(const BarzelEntityRangeCombo &data) {
		os << "<entrange>" << "</entrange>";
	}
};
}

std::ostream& BarzStreamerXML::print(std::ostream &os)
{
	os << "<barz>" << std::endl;
	const BarzelBeadChain &bc = barz.getBeads();
	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
		const BarzelBead &bead = *bli;
		if (bead.isBlank()) {
			os << " ";
		} else if (bead.isAtomic()) {
			os << "    ";
			AtomicVisitor v(os, universe);
			boost::apply_visitor(v, bead.getAtomic()->dta);
			os << std::endl;
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
	os << "</barz>" << std::endl;
	return os;
}

}
