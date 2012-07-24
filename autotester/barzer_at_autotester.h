#pragma once

#include <istream>
#include <deque>
#include <boost/unordered_map.hpp>
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
            Test( const std::string& x, const boost::property_tree::ptree& y )  : 
                m_query(x), m_refResponse(y) {}
		};
		typedef std::deque<Test> TestSet_t;
		boost::unordered_map<uint32_t, TestSet_t> m_tests;
	public:
		Autotester(std::istream& source);
	};
}
}
