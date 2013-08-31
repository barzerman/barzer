#include "barzer_bson.h"
#include <fstream>
#include <string>
#include <iterator>
#include <chrono>
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
		else if (child.first == "root")
			addDoc(child.second, enc);
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

void perfTest()
{
	std::function<void ()> funcs[]
	{
		[] () -> void
		{
			bson::Encoder enc;
			for (size_t i = 0; i < 1000; ++i)
				enc.encode_string("this is a dumb and long string", "strname");
			enc.finalize();
		}
	};
	/*
	std::function<void ()> funcs[]
	{
		[] () -> void
		{
			bson::Encoder enc;
			enc.encode_int32(100, "test");
			enc.finalize();
		},
		[] () -> void
		{
			bson::Encoder enc;
			enc.encode_int32(100, "test");
			enc.encode_int32(200, "rest");
			enc.encode_string("this is a dumb and long string", "strname");
			enc.finalize();
		},
		[] () -> void
		{
			bson::Encoder enc;
			for (size_t i = 0; i < 1000; ++i)
				enc.encode_string("this is a dumb and long string", "strname");
			enc.finalize();
		},
		[] () -> void
		{
			bson::Encoder enc;
			for (size_t i = 0; i < 1000; ++i)
			{
				enc.encode_int32(100, "test");
				enc.encode_int32(200, "rest");
				enc.encode_string("this is a dumb and long string", "strname");
			}
			enc.finalize();
		},
		[] () -> void
		{
			bson::Encoder enc;
			for (size_t i = 0; i < 1000; ++i)
				enc.document_start(false, "doc");
			enc.encode_string("some typical string", "strname");
			for (size_t i = 0; i < 1000; ++i)
				enc.document_end();
			enc.finalize();
		}
	};
	*/

	std::cout << "testing performance..." << std::endl;
	const size_t iter = 100000;
	for (auto f : funcs)
	{
		const auto start = std::chrono::system_clock::now();
		for (size_t i = 0; i < iter; ++i)
			f();
		const auto end = std::chrono::system_clock::now();

		std::cout << "got " << iter << " iterations in "
				<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
				<< " ms" << std::endl;
	}
}

std::vector<uint8_t> genBSON(const pt::ptree& pt)
{
	bson::Encoder crapson;
	addDoc(pt, crapson);
	return crapson.finalize();
}

int main(int argc, char **argv)
{
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
		bool same = data == s.m_binary;
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

	perfTest();
}
