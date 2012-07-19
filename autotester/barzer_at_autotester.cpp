#include "barzer_at_autotester.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace proptree = boost::property_tree;

namespace barzer
{
namespace autotester
{
	Autotester::Autotester (std::istream& source)
	{
		proptree::ptree tree;
		proptree::read_xml(source, tree);

		for (const auto& userElem : tree.get_child("tests"))
		{
			if (userElem.first != "user")
			{
				std::cerr << "unknown tag" << userElem.first << std::endl;
				continue;
			}
			const uint32_t userId = userElem.second.get("<xmlattr>.id", 0);

			TestSet_t tests = m_tests[userId];

			for (const auto& testElem : userElem.second)
			{
				if (testElem.first != "test")
					continue;

				tests.push_back( 
                    TestSet_t::value_type( 
                    testElem.second.get<std::string>("request"), 
                    testElem.second.get_child("outxml") 
                    ) 
                );
			}

			m_tests[userId] = tests;
		}
	}
}
}
