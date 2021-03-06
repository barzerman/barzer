#pragma once

#include <map>
#include <cstddef>
#include <stdint.h>

namespace barzer
{
class Barz;
class GlobalPools;

namespace autotester
{
	enum class BeadMatchType
	{
		Value,
		Type,
		Ignore
	};

	enum class ClassMatchType
	{
		Primary,
		WithSubclass,
		Full
	};

	class BeadMatchOptions
	{
		BeadMatchType m_matchType;
		ClassMatchType m_classMatchType;
		bool m_skipFluff;
		bool m_matchERCRange;
	public:
		BeadMatchOptions();

		BeadMatchType matchType() const;
		BeadMatchOptions& setMatchType(BeadMatchType);

		ClassMatchType classMatchType() const;
		BeadMatchOptions& setClassMatchType(ClassMatchType);

		bool skipFluff() const;
		BeadMatchOptions& setSkipFluff(bool);

		bool matchERCRange() const;
		BeadMatchOptions& setMatchERCRange(bool);
	};

	class CompareSettings
	{
		BeadMatchOptions m_defaultOptions;
		std::map<size_t, BeadMatchOptions> m_posSettings;

		bool m_removeFluff;
	public:
		CompareSettings(bool removeFluff = true, const BeadMatchOptions& = BeadMatchOptions());

		void setBeadOption(size_t pos, const BeadMatchOptions&);
		const BeadMatchOptions& getOptions(size_t idx) const;

		bool removeFluff() const;
	};

	struct ParseContext
	{
		GlobalPools& m_gp;
		uint32_t m_userId;

		ParseContext (GlobalPools& gp, uint32_t userId)
		: m_gp(gp)
		, m_userId(userId)
		{
		}
	};

	uint16_t matches(const Barz& pattern, const Barz& result, const CompareSettings& = CompareSettings());
	uint16_t matches(const char *pattern, size_t patSize,
			const char *result, size_t resSize,
			const ParseContext&, const CompareSettings& = CompareSettings());
}
}
