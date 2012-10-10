#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bpt = boost::property_tree;

int main (int argc, char **argv)
{
	if (argc < 2)
		std::cerr << "Usage: " << argv [0] << " map.osm" << std::endl;

	bpt::ptree tree;
	std::cout << "Reading XML..." << std::endl;
	bpt::read_xml(argv[1], tree);
	std::cout << "Done." << std::endl;

	for (const auto& v : tree.get_child("osm"))
	{
		if (v.first != "node")
			continue;

		const auto& tags = v.second.get_child_optional("tag");
		if (!tags)
			continue;

		std::cout << "got tags for entity " << v.second.get<double>("<xmlattr>.lon") << " " << v.second.get<double>("<xmlattr>.lat") << std::endl;
		for (const auto& tag : *tags)
		{
			std::cout << tag.second.get<std::string>("k") << " => " << tag.second.get<std::string>("v") << std::endl;
		}
	}

	while (true);
}
