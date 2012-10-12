#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <expat.h>
#include <string.h>

class Parser
{
	XML_Parser m_expat;

	enum class State
	{
		AwaitingNode,
		AwaitingTags
	} m_state;

	double m_lat, m_lon;
	std::map<std::string, std::string> m_tags;
	std::set<std::string> m_keys;

	std::map<std::string, std::set<std::string>> m_keyvals;

	std::ostream& m_ostr;
public:
	Parser(std::ostream&);

	void parse(std::istream& istr)
	{
		const size_t bufSize = 1048576;
		char buf[bufSize];
		bool end = false;
		bool done = false;
		do
		{
			istr.read (buf, bufSize - 1);
			size_t len = istr.gcount();
			done = len < bufSize - 1;
			if (!XML_Parse(m_expat, buf, len, done))
			{
				std::cerr << "parse failure" << std::endl;
				return;
			}
		} while (!done);
	}

	void dumpStats() const
	{
		std::cout << "Tags:" << std::endl;
		for (const auto& str : m_keys)
		{
			std::cout << str << std::endl;
			for (const auto& val : m_keyvals.at(str))
				std::cout << "\t" << val << std::endl;
		}
	}

	void startElem(const XML_Char *n, const XML_Char **a)
	{
		switch(m_state)
		{
		case State::AwaitingNode:
			if (!strcmp(n, "node"))
				handleNode(a);
			break;
		case State::AwaitingTags:
			if (!strcmp(n, "tag"))
				handleTag(a);
			break;
		}
	}

	void endElem(const XML_Char *n)
	{
		if (m_state == State::AwaitingTags && !strcmp(n, "node"))
		{
			m_state = State::AwaitingNode;
			flushNode();
		}
	}
private:
	void handleNode(const XML_Char **a)
	{
		m_state = State::AwaitingTags;

		const int attrCount = XML_GetSpecifiedAttributeCount(m_expat);
		for (int i = 0; i < attrCount; i += 2)
		{
			if (!strcmp(a[i], "lon"))
				m_lon = atof(a[i + 1]);
			else if (!strcmp(a[i], "lat"))
				m_lat = atof(a[i + 1]);
		}
	}

	void handleTag(const XML_Char **a)
	{
		const int attrCount = XML_GetSpecifiedAttributeCount(m_expat);
		std::string name, val;
		for (int i = 0; i < attrCount; i += 2)
		{
			if (*a[i] == 'k')
				name.assign(a[i + 1]);
			else if (*a[i] == 'v')
				val.assign(a[i + 1]);
		}
		m_tags[name] = val;
		m_keys.insert(name);
		m_keyvals[name].insert(val);
	}

	void flushNode()
	{
		if (m_tags.empty())
			return;

		/*
		std::cout << "items for node " << m_lat << " " << m_lon << std::endl;
		for (const auto& p : m_tags)
			std::cout << p.first << " => " << p.second << "; ";
		std::cout << std::endl;
		*/
		auto tags = m_tags;
		m_tags.clear();

		if (tags.find("traffic_sign") != tags.end())
			return;

		std::vector<std::string> possibleNames;
		auto tryTag = [&possibleNames, &tags](const std::string& tagName)
		{
			const auto name = tags.find(tagName);
			if (name != tags.end())
				possibleNames.push_back(name->second);
		};
		tryTag("name");
		tryTag("alt_name");
		tryTag("amenity");
		tryTag("shop");
		tryTag("operator");

		if (possibleNames.empty())
			return;

		m_ostr << "\t<stmt>\n\t\t<pat>";
		if (possibleNames.size() > 1)
			m_ostr << "<any>";
		for (const auto& str : possibleNames)
			m_ostr << "<t>" << str << "</t>";
		if (possibleNames.size() > 1)
			m_ostr << "</any>";
		m_ostr << "</pat>" << std::endl << "\t\t<tran>";
		m_ostr << "<mkent c='0' s='0' p='" << m_lat << ", " << m_lon << "' ";
		m_ostr << "/>\n\t</stmt>" << std::endl;
	}
};

namespace
{
	void startElement(void *ud, const XML_Char *n, const XML_Char **a)
	{
		static_cast<Parser*>(ud)->startElem(n, a);
	}

	void endElement(void *ud, const XML_Char *n)
	{
		static_cast<Parser*>(ud)->endElem(n);
	}
}

Parser::Parser(std::ostream& ostr)
: m_expat(XML_ParserCreate(NULL))
, m_state(State::AwaitingNode)
, m_ostr(ostr)
{
	XML_SetUserData (m_expat, this);
	XML_SetElementHandler (m_expat, startElement, endElement);
}

int main (int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "Usage: " << argv [0] << " map.osm outfile.xml" << std::endl;
		return 1;
	}

	std::ifstream istr(argv[1]);
	std::ofstream ostr(argv[2]);

	ostr << "<?xml version='1.0' encoding='UTF-8'?>" << std::endl;
	ostr << "<stmset xmlns:xsi='http://www.w3.org/2000/10/XMLSchema-instance' xmlns='http://www.barzer.net/barzel/0.1'>" << std::endl;

	static const std::map<std::string, std::vector<std::string>> amenitySynonyms =
	{
		{ "atm", { "банкомат" } },
		{ "doctor", { "врач", "больница" } },
		{ "dentist", { "врач", "зубной</t><t>врач"} },
		{ "kindergarten", { "детский</t><t>сад", "дошкольное</t><t>учреждение" } }
	};
	ostr << "\t<!-- amenity synonyms block -->\n";
	for (const auto& pair : amenitySynonyms)
	{
		for (const auto& syn : pair.second)
		{
			ostr << "\t<stmt>";
			ostr << "\n\t\t<pat><t>" + syn + "</t></pat>";
			ostr << "\n\t\t<tran><t>" << pair.first << "</t></tran>";
			ostr << "\n\t</stmt>\n";
		}
	}

	ostr << "\n\t<!-- generated rules -->\n";

	Parser p(ostr);
	p.parse(istr);
	std::cout << "DUMPING STATS" << std::endl;
	p.dumpStats();

	ostr << "</stmset>";
}
