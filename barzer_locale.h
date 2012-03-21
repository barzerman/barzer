/*
 * barzer_date_util.h
 *
 *  Created on: Mar 14, 2012
 *      Author: 0xd34df00d
 */

#ifndef BARZER_LOCALE_H_
#define BARZER_LOCALE_H_
#include <vector>
#include <boost/shared_ptr.hpp>
#include <string>

namespace barzer
{
	class BarzerLocale;
	typedef boost::shared_ptr<BarzerLocale> BarzerLocale_ptr;
	
	class BarzerLocale
	{
		std::string m_name;
	public:
		static BarzerLocale_ptr makeLocale(const std::string&);

		virtual ~BarzerLocale();

		virtual char realSeparator() const = 0;
		virtual std::string dateTimeFormat() const = 0;
	};
}

#endif
