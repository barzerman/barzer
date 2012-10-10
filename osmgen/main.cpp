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
	std::vector<std::pair<std::string, std::string>> m_tags;
	std::set<std::string> m_keys;

	std::map<std::string, std::set<std::string>> m_keyvals;
public:
	Parser();

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
		std::pair<std::string, std::string> tag;
		for (int i = 0; i < attrCount; i += 2)
		{
			if (*a[i] == 'k')
				tag.first.assign(a[i + 1]);
			else if (*a[i] == 'v')
				tag.second.assign(a[i + 1]);
		}
		m_tags.push_back(tag);
		m_keys.insert(tag.first);
		m_keyvals[tag.first].insert(tag.second);
	}

	void flushNode()
	{
		if (m_tags.empty())
			return;

		std::cout << "items for node " << m_lat << " " << m_lon << std::endl;
		for (const auto& p : m_tags)
			std::cout << p.first << " => " << p.second << "; ";
		std::cout << std::endl;
		m_tags.clear();
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

Parser::Parser()
: m_expat(XML_ParserCreate(NULL))
, m_state(State::AwaitingNode)
{
	XML_SetUserData (m_expat, this);
	XML_SetElementHandler (m_expat, startElement, endElement);
}

int main (int argc, char **argv)
{
	if (argc < 2)
		std::cerr << "Usage: " << argv [0] << " map.osm" << std::endl;

	std::ifstream istr(argv[1]);

	Parser p;
	p.parse(istr);
	std::cout << "DUMPING STATS" << std::endl;
	p.dumpStats();
}
