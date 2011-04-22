/*
 * barz_server_response.cpp
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#include "barz_server_response.h"
#include "ay/ay_logger.h"
#include <sstream>

namespace barzer {

namespace {

// this probably won't work with utf-8 properly.
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


class AtomicVisitor : public boost::static_visitor<> {
	std::ostream &_os;
	StoredUniverse &_u;
public:
	AtomicVisitor(std::ostream &os, StoredUniverse &u) : _os(os), _u(u) {}

	void operator()(const BarzerLiteral &data) {
		if (data.isBlank()) return;
		_os << "<token>";
		switch(data.getType()) {
		case BarzerLiteral::T_STRING: {
			std::string s = _u.getStringPool().resolveId(data.getId());
			xmlEscape(s, _os);
		}
		case BarzerLiteral::T_COMPOUND: // shrug
			break;
		case BarzerLiteral::T_STOP: // shrug
			break;
		case BarzerLiteral::T_PUNCT:
			_os << (char)data.getId(); // cough. need to somehow make this localised
			break;
		case BarzerLiteral::T_BLANK:
			_os << " ";
			break;
		default:
			AYLOG(ERROR) << "Unknown literal type";
			_os << "<error>unknown literal type</error>";
		}
		_os << "</token>";
	}
	void operator()(const BarzerString &data) {
		_os << "<token>";
		xmlEscape(data.getStr(), _os) << "</token>";
	}
	void operator()(const BarzerNumber &data) {
		_os << "<number>";
		data.print(_os) << "</number>";
	}
	void operator()(const BarzerDate &data) {
		_os << "<date>";
		data.print(_os) << "</number>";
	}
	void operator()(const BarzerTimeOfDay &data) {
		_os << "<time>";
		data.print(_os) << "</time>";
	}
	void operator()(const BarzerRange &data) {
		_os << "<range>";
		_os << "</range>";


	} // not sure how to properly deconstruct this yet
	void operator()(const BarzerEntityList &data) {
		_os << "<entlist>";
		_os << "</entlist>";
	}
	void operator()(const BarzelEntityRangeCombo &data) {
		_os << "<entrange>";
		_os << "</entrange>";
	}
};
}

std::ostream& BarzStreamerXML::print(std::ostream & os)
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
