#include "barzer_bson.h"
#include <fstream>
#include <string>
#include <iterator>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace pt = boost::property_tree;

struct Sample
{
	std::string m_name;
	pt::ptree m_xml;
	std::vector<uint8_t> m_binary;
};

void addDoc(const pt::ptree& pt, bson::Encoder& enc)
{
	for (const auto& child : pt)
	{
		const auto& nameStr = child.second.get("<xmlattr>.name", std::string());
		const auto name = nameStr.c_str();
		const auto value = child.second.get("<xmlattr>.value", std::string()).c_str();

		if (child.first == "doc")
		{
			enc.document_start(false, nameStr.empty() ? nullptr : name);
			addDoc(child.second, enc);
			enc.document_end();
		}
		else if (child.first == "int32")
			enc.encode_int32(boost::lexical_cast<int32_t>(value), name);
		else if (child.first == "bool")
			enc.encode_bool(boost::lexical_cast<bool>(value), name);
		else if (child.first == "double")
			enc.encode_double(boost::lexical_cast<double>(value), name);
		else if (child.first == "str")
			enc.encode_string(value, name);
		else
			throw std::runtime_error("unknown type: " + child.first);
	}
}

std::vector<uint8_t> genBSON(const pt::ptree& pt)
{
	std::vector<uint8_t> buf;
	bson::Encoder crapson(buf);

	addDoc(pt, crapson);

	return buf;
}

int main(int argc, char **argv)
{
	if (argc == 1)
		return 1;

	std::vector<Sample> samples;

	for (int i = 1; i < argc; ++i)
	{
		const std::string name(argv[i]);
		std::ifstream xmlFile(name + ".xml");

		pt::ptree pt;
		pt::xml_parser::read_xml(xmlFile, pt);

		std::ifstream binIstr(name + ".bson", std::ios_base::in | std::ios_base::binary);
		binIstr.unsetf(std::ios::skipws);

		Sample s { name, pt, {} };
		std::copy(std::istream_iterator<uint8_t>(binIstr), std::istream_iterator<uint8_t>(),
				std::back_inserter(s.m_binary));

		samples.push_back(s);
	}

	for (const auto& s : samples)
	{
		std::cout << "testing " << s.m_name << "... ";

		const auto& data = genBSON(s.m_xml);
		bool same = data.size() == s.m_binary.size();
		std::cout << (same ? "PASSED" : "FAILED");
		std::cout << std::endl;

		if (!same)
		{
			std::ofstream ostr(s.m_name + ".bson.out");
			for (const auto& c : data)
				ostr << c;

			std::ofstream ostr2(s.m_name + ".bson.stock");
			for (const auto& c : s.m_binary)
				ostr2 << c;
		}
	}
}
