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
static std::string xmlEscape(const std::string &src) {
	std::ostringstream ret;
	for(std::string::const_iterator i = src.begin(); i != src.end(); ++i) {
		char c = (char)*i;
		switch(c) {
		case '&': ret << "&amp;"; break;
		case '<': ret << "&lt;"; break;
		case '>': ret << "&gt;"; break;
		case '"': ret << "&quot;"; break;
		case '\'': ret << "&apos;"; break;
		default: ret << c;
		}
	}
	return ret.str();
}


class AtomicVisitor : public boost::static_visitor<> {
	std::ostream &_os;
	StoredUniverse &_u;
public:
	AtomicVisitor(std::ostream &os, StoredUniverse &u) : _os(os), _u(u) {}

	void operator()(const BarzerLiteral &data) {
		_os << "<literal>";
		switch(data.getType()) {
		case BarzerLiteral::T_STRING: {
			std::string s = _u.getStringPool().resolveId(data.getId());
			_os << xmlEscape(s);
		}
		case BarzerLiteral::T_COMPOUND: // shrug
			break;
		case BarzerLiteral::T_STOP: // shrug
			break;
		case BarzerLiteral::T_PUNCT:
			_os << (char)data.getId(); // cough. need to somehow make this localised
			break;
		case BarzerLiteral::T_BLANK:
			_os << "<blank />";
			break;
		default:
			AYLOG(ERROR) << "Unknown literal type";
			_os << "<error>unknown literal type</error>";
		}
		_os << "</literal>";
	}
	void operator()(const BarzerString &data) {
		_os << "<string>" << xmlEscape(data.getStr()) << "</string>";
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
	void operator()(const BarzerRange &data) {} // not sure how to properly deconstruct this yet
	void operator()(const BarzerEntityList &data) {}
	void operator()(const BarzelEntityRangeCombo &data) {}
};
}

std::ostream& BarzStreamerXML::print(std::ostream & os)
{
	const BarzelBeadChain &bc = barz.getBeads();
	for (BeadList::const_iterator bli = bc.getLstBegin(); bc.isIterNotEnd(bli); ++bli) {
		const BarzelBead &bead = *bli;
		if (bead.isBlank()) {
			os << "<blank />";
		} else if (bead.isAtomic()) {
			AtomicVisitor v(os, universe);
			boost::apply_visitor(v, bead.getAtomic()->dta);
		} else if (bead.isExpression()) {
			// not quite sure what to do with it yet
			// const BarzelBeadExpression *bbe = bead.getExpression();
		} else {
			AYLOG(ERROR) << "Unknown bead type";
			// then wtf is this
		}
	}
	return os;
}

}
