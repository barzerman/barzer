/*
 * barzer_date_util.h
 *
 *  Created on: Mar 14, 2012
 *      Author: 0xd34df00d
 */

#include "barzer_locale.h"
#include <boost/optional.hpp>
#include <ay/ay_logger.h>

namespace barzer
{
	class BarzerLocaleEN : public BarzerLocale
	{
	public:
		char realSeparator() const
		{
			return '.';
		}

		std::string dateTimeFormat() const
		{
			return "MM/DD/YYYY";
		}
	};

	class BarzerLocaleRU : public BarzerLocale
	{
	public:
		char realSeparator() const
		{
			return ',';
		}

		std::string dateTimeFormat() const
		{
			return "DD.MM.YYYY";
		}
	};

	class BarzerLocaleAugmented : public BarzerLocale
	{
		BarzerLocale_ptr m_base;

		boost::optional<char> m_realSeparator;
		boost::optional<std::string> m_dateTimeFormat;
	public:
		BarzerLocaleAugmented(BarzerLocale_ptr base)
		: m_base(base)
		{
		}

#define AUGMENTER(type,name) \
		void augment_##name(type var) \
		{ \
			m_##name.reset(var); \
		}

		AUGMENTER(char, realSeparator);
		AUGMENTER(std::string, dateTimeFormat);

#define AUG_GETTER(type,name) \
		inline type name() const \
		{ \
			if (m_##name) \
				return *m_##name; \
			else \
				return m_base->name(); \
		}

		AUG_GETTER(char, realSeparator);
		AUG_GETTER(std::string, dateTimeFormat);

#define DECLARE_FUN(type,name) \
		AUGMENTER(type, name) \
		AUG_GETTER(type, name)

#undef AUGMENTER
#undef AUG_GETTER
	};

	BarzerLocale::~BarzerLocale()
	{
	}

	BarzerLocale_ptr BarzerLocale::makeLocale(const std::string& name)
	{
		if (name == "en" || !name.size())
			return BarzerLocale_ptr(new BarzerLocaleEN());
		else if (name == "ru")
			return BarzerLocale_ptr(new BarzerLocaleRU());
		else
		{
			AYLOG(WARNING) << "unknown locale name" << name;
			return BarzerLocale_ptr(new BarzerLocaleEN());
		}
	}
}
