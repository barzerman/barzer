/*
 * umlaut_python.cpp
 *
 *  Created on: 23.06.2011
 *      Author: polter
 */

#include <boost/python.hpp>
#include "umlaut.h"
#include <pyconfig.h>

using namespace boost::python;

const std::string stripDiacritics(const std::string &src) {
	std::string out;
	umlautsToAscii(out, src.c_str());
	return out;
}
/*
BOOST_PYTHON_MODULE(pybarzer)
{
    def("stripDiacritics", stripDiacritics);

} */
