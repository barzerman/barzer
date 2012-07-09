#pragma once

#include <istream>
#include <deque>
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>

namespace barzer
{
namespace autotester
{
	class Autotester
	{
		struct Test
		{
			std::string m_query;
			boost::property_tree::ptree m_refResponse;
		};
		typedef std::deque<Test> TestSet_t;
		std::unordered_map<uint32_t, TestSet_t> m_tests;
	public:
		Autotester(std::istream& source);
	};
}
}
