#include "barzer_at_comparators.h"
#include <algorithm>
#include <iterator>
#include <cmath>
#include <iostream>
#include "../barzer_barz.h"
#include "../barzer_barzxml.h"
#include "../barzer_server_request.h"
#include "../barzer_universe.h"

extern "C"
{
#include <expat.h>
}

namespace barzer
{
namespace autotester
{
	namespace
	{
		void startElement(void *ud, const XML_Char *n, const XML_Char **a);
		void endElement(void *ud, const XML_Char *n);
		void charDataHandle( void * ud, const XML_Char *str, int len);

		class EnbarzParser
		{
			Barz& m_result;

			XML_Parser m_parser;
			std::stringstream m_barzXMLOstr;
			BarzXMLParser m_barzXML;
		public:
			EnbarzParser(Barz& result, StoredUniverse& u)
			: m_result(result)
			, m_parser(XML_ParserCreate(NULL))
			, m_barzXML(m_result, m_barzXMLOstr, u.getGlobalPools(), u)
			{
				m_barzXML.setInternStrings(true);
				m_barzXML.setPerformCleanup(false);

				XML_SetUserData(m_parser, this);
				XML_SetElementHandler(m_parser, &startElement, &endElement);
				XML_SetCharacterDataHandler(m_parser, &charDataHandle);
			}

			~EnbarzParser()
			{
				XML_ParserFree(m_parser);
			}

			int operator()(const char *str)
			{
				return XML_Parse(m_parser, str, std::strlen(str), true);
			}

			void start(const char *n, const char **a)
			{
				m_barzXML.takeTag(n, a, XML_GetSpecifiedAttributeCount(m_parser), true);
			}

			void end(const char *n)
			{
				m_barzXML.takeTag(n, 0, 0, false);
			}

			void cdata(const char *str, int len)
			{
				m_barzXML.takeCData(str, len);
			}
		};

		void startElement(void *ud, const XML_Char *n, const XML_Char **a)
		{
			static_cast<EnbarzParser*>(ud)->start(n, a);
		}

		void endElement(void *ud, const XML_Char *n)
		{
			static_cast<EnbarzParser*>(ud)->end(n);
		}

		void charDataHandle (void *ud, const XML_Char *str, int len)
		{
			static_cast<EnbarzParser*>(ud)->cdata(str, len);
		}
	}

	BeadMatchOptions::BeadMatchOptions()
	: m_matchType(BeadMatchType::Value)
	, m_classMatchType(ClassMatchType::Full)
	, m_skipFluff(true)
	{
	}

	BeadMatchType BeadMatchOptions::matchType() const
	{
		return m_matchType;
	}

	BeadMatchOptions& BeadMatchOptions::setMatchType(BeadMatchType type)
	{
		m_matchType = type;
		return *this;
	}

	ClassMatchType BeadMatchOptions::classMatchType() const
	{
		return m_classMatchType;
	}

	BeadMatchOptions& BeadMatchOptions::setClassMatchType(ClassMatchType type)
	{
		m_classMatchType = type;
		return *this;
	}

	bool BeadMatchOptions::skipFluff() const
	{
		return m_skipFluff;
	}

	BeadMatchOptions& BeadMatchOptions::setSkipFluff(bool skip)
	{
		m_skipFluff = skip;
		return *this;
	}

	CompareSettings::CompareSettings(bool removeFluff, const BeadMatchOptions& defOpts)
	: m_defaultOptions(defOpts)
	, m_removeFluff(removeFluff)
	{
	}

	void CompareSettings::setBeadOption(size_t pos, const BeadMatchOptions& opts)
	{
		m_posSettings[pos] = opts;
	}

	const BeadMatchOptions& CompareSettings::getOptions(size_t idx) const
	{
		auto pos = m_posSettings.find(idx);
		return pos == m_posSettings.end() ?
				m_defaultOptions :
				pos->second;
	}

	bool CompareSettings::removeFluff() const
	{
		return m_removeFluff;
	}

	namespace
	{
		template<typename LeftElem, typename RightElem, typename Alloc, template<typename, typename> class Container>
		inline Container<std::pair<LeftElem, RightElem>, Alloc> zip(const Container<LeftElem, Alloc>& c1, const Container<RightElem, Alloc>& c2)
		{
			decltype(zip(c1, c2)) result;
			auto pIter = std::begin(c1), pEnd = std::end(c1);
			auto rIter = std::begin(c2), rEnd = std::end(c2);
			while (pIter != pEnd && rIter != rEnd)
				result.push_back(std::make_pair(*pIter++, *rIter++));
			return result;
		}

		namespace Scores
		{
			const int RootLengthFailure = 100;

			const int StringsEqFailure = 1;
			const int EListLengthFailure = 3;
			const int EListClassFailure = 3;
			const int SECPrimaryFailure = 4;
			const int SECSecondaryFailure = 5;
			const int GenericValueMismatch = 1;
			const int GenericTypeMismatch = 20;
		}

		struct BeadComparator : public boost::static_visitor<uint16_t>
		{
			const BeadMatchOptions& m_opts;

			BeadComparator(const BeadMatchOptions& opts)
			: m_opts(opts)
			{
			}

			uint16_t operator()(const BarzerString& str1, const BarzerString& str2) const
			{
				if (m_opts.skipFluff() && str1.isFluff() && str2.isFluff())
					return 0;

				if (m_opts.matchType() != BeadMatchType::Value || str1.getStr() == str2.getStr())
					return 0;
				else
					return Scores::StringsEqFailure;
			}

			uint16_t operator()(const BarzerEntityList& left, const BarzerEntityList& right) const
			{
				if (m_opts.matchType() != BeadMatchType::Value)
					return 0;

				uint16_t result = 0;
				if (left.size() != right.size())
					result += Scores::EListLengthFailure;

				if ((*this) (left.getClass (), right.getClass()))
					result += Scores::EListClassFailure;

				const auto& zipped = zip(left.getList(), right.getList());
				result += std::accumulate(zipped.begin(), zipped.end(), 0,
						[this] (uint16_t val, decltype(zipped.front()) item) { return val + (*this) (item.first, item.second); });
				return result;
			}

			uint16_t operator()(const StoredEntityClass& sec1, const StoredEntityClass& sec2) const
			{
				if (m_opts.matchType() != BeadMatchType::Value)
					return 0;

				uint16_t score = 0;
				if (sec1.ec != sec2.ec)
					score += Scores::SECPrimaryFailure;

				if (m_opts.classMatchType() != ClassMatchType::Primary &&
						sec1.subclass != sec2.subclass)
					score += Scores::SECSecondaryFailure;

				return score;
			}

			uint16_t operator()(const StoredEntityUniqId& left, const StoredEntityUniqId& right) const
			{
				if (m_opts.matchType() != BeadMatchType::Value)
					return 0;

				uint16_t score = (*this) (left.getClass (), right.getClass());

				if (m_opts.classMatchType() == ClassMatchType::Full &&
					left.getTokId() != right.getTokId())
					score += Scores::StringsEqFailure;

				return score;
			}

			uint16_t operator()(const BarzelBeadAtomic& at1, const BarzelBeadAtomic& at2) const
			{
				return boost::apply_visitor(*this, at1.getData(), at2.getData());
			}

			template<typename T>
			uint16_t operator()(const T& t1, const T& t2) const
			{
				if (m_opts.matchType() == BeadMatchType::Value)
					return t1 == t2 ? 0 : Scores::GenericValueMismatch;
				else
					return 0;
			}

			template<typename T, typename U>
			uint16_t operator()(T, U) const
			{
				return m_opts.matchType() == BeadMatchType::Ignore ? 0 : Scores::GenericTypeMismatch;
			}
		};
	}

	int parseXML(const char *string, Barz& barz, const ParseContext& ctx)
	{
		return EnbarzParser(barz, *ctx.m_gp.getUniverse(ctx.m_userId))(string);
	}

	uint16_t matches(const Barz& pattern, const Barz& result, const CompareSettings& cmpSettings)
	{
		auto pBeads = pattern.getBeadList();
		auto rBeads = result.getBeadList();

		auto skipBead = [&cmpSettings] (decltype(pBeads.begin()) iter) -> bool
		{
			if (!cmpSettings.removeFluff())
				return false;
			auto str = iter->get<BarzerString>();
			return str ? str->isFluff() : false;
		};

		if (pBeads.size() != rBeads.size())
			return Scores::RootLengthFailure;

		auto pIter = pBeads.begin(), pEnd = pBeads.end();
		auto rIter = rBeads.begin(), rEnd = rBeads.end();
		size_t pos = 0;
		uint16_t score = 0;
		while (pIter != pEnd && rIter != rEnd)
		{
			score += boost::apply_visitor(BeadComparator(cmpSettings.getOptions(pos++)),
					pIter->getBeadData(), rIter->getBeadData());

			do
				++pIter;
			while (pIter != pEnd && skipBead(pIter));
			do
				++rIter;
			while (rIter != rEnd && skipBead(rIter));
		}

		return 100 * (std::atan(static_cast<double>(score) / 10) * 2 / 3.14160);
	}

	uint16_t matches(const char *pattern, const char *result, const ParseContext& ctx, const CompareSettings& settings)
	{
		Barz pattBarz, resBarz;
		parseXML(pattern, pattBarz, ctx);
		parseXML(result, resBarz, ctx);
		return matches(pattBarz, resBarz, settings);
	}
}
}
