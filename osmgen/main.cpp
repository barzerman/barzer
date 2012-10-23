#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <expat.h>
#include <string.h>

namespace
{
	void replace(std::string& str, char what, const char *with)
	{
		auto pos = 0;
		const auto withLen = strlen(with);
		while ((pos = str.find(what, pos)) != std::string::npos)
		{
			str.replace(pos, 1, with);
			pos += withLen;
		}
	}

	void xmlEscape(std::string& str)
	{
		replace(str, '&', "&amp;");
		replace(str, '<', "&lt;");
		replace(str, '>', "&gt;");
	}

	void fillAlts(const std::string& word, std::vector<std::string>& out)
	{
		static const std::map<std::string, std::vector<std::string>> amenitySynonyms =
		{
			{ "air-raid_shelter", { "бомбоубежище" } },
			{ "atm", { "банкомат" } },
			{ "bank", { "банк" } },
			{ "bench", { "лавочка", "лавка", "скамейка" } },
			{ "bureau_de_change", { "обменный</t><t>пункт", "обмен" } },
			{ "bus_station", { "автобусная</t><t>остановка", "автобус" } },
			{ "cafe", { "кафе" } },
			{ "car_wash", { "автомойка" } },
			{ "cinema", { "кино", "кинотеатр" } },
			{ "college", { "колледж", "ПТУ" } },
			{ "courthouse", { "суд" } },
			{ "drinking_water", { "вода" } },
			{ "driving_school", { "автокурс", "курс</t><t>вождения" } },
			{ "embassy", { "посольство" } },
			{ "doctor", { "врач", "больница" } },
			{ "dentist", { "врач", "зубной</t><t>врач"} },
			{ "fast_food", { "забегаловка", "фастфуд" } },
			{ "fountain", { "фонтан" } },
			{ "fuel", { "заправка", "топливо", "заправочная</t><t>станция" } },
			{ "hospital", { "госпиталь", "больница" } },
			{ "ice_cream", { "мороженое" } },
			{ "library", { "библиотека" } },
			{ "nightclub", { "ночной</t><t>клуб", "клуб" } },
			{ "parking", { "парковка", "стоянка", "автостоянка" } },
			{ "payment_terminal", { "платежный</t><t>терминал", "оплата" } },
			{ "pharmacy", { "аптека" } },
			{ "police", { "милиция", "полиция" } },
			{ "post_office", { "почта", "почтовое</t><t>отделение" } },
			{ "pub", { "бар", "паб" } },
			{ "recycling", { "урна", "мусор" } },
			{ "restaurant", { "ресторан" } },
			{ "school", { "школа" } },
			{ "shelter", { "приют" } },
			{ "swimming_pool", { "бассейн" } },
			{ "taxi", { "такси" } },
			{ "telephone", { "телефон", "таксофон" } },
			{ "theatre", { "театр" } },
			{ "toilet", { "туалет" } },
			{ "university", { "вуз", "университет", "институт" } },
			{ "waste_basket", { "урна", "мусорная</t><t>корзина" } },
			{ "waste_disposal", { "урна", "мусорная</t><t>корзина" } },
			{ "kindergarten", { "детский</t><t>сад", "дошкольное</t><t>учреждение" } },
			{ "place_of_worship", { "храм", "церковь", "мечеть" } }
		};

		const auto pos = amenitySynonyms.find(word);
		if (pos != amenitySynonyms.end())
			out = pos->second;
	}
}

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

	std::map<std::string, std::map<std::string, size_t>> m_keyvals;

	std::ostream& m_ostr;

	size_t m_numEntities;
	size_t m_entId;
public:
	Parser(std::ostream&, size_t&);

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
		std::cout << "Entities generated: " << m_numEntities << std::endl;
		std::cout << "Tags:" << std::endl;
		for (const auto& str : m_keys)
		{
			std::cout << str << std::endl;
			for (const auto& val : m_keyvals.at(str))
				if (val.second > 1)
					std::cout << "\t" << val.second << "\t\t" << val.first << std::endl;
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
		m_keyvals[name][val]++;
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

		std::set<std::string> possibleNames;
		auto tryTag = [&possibleNames, &tags](const std::string& tagName)
		{
			const auto name = tags.find(tagName);
			if (name != tags.end())
			{
				auto val = name->second;
				xmlEscape(val);
				replace(val, ' ', "</t><t>");

				std::vector<std::string> altNames;
				fillAlts(val, altNames);

				val = "<t>" + val + "</t>";
				possibleNames.insert(val);

				for (const auto& name : altNames)
					possibleNames.insert("<t>" + name + "</t>");
			}
		};
		tryTag("name");
		tryTag("name:en");
		tryTag("name:ru");
		tryTag("alt_name");
		tryTag("amenity");
		tryTag("shop");
		tryTag("operator");
		tryTag("old_name");
		tryTag("official_name");
		tryTag("int_name");

		if (possibleNames.empty())
			return;

		++m_numEntities;

		m_ostr << "\t<stmt n='" << ++m_entId << "'>\n\t\t<pat>";
		if (possibleNames.size() > 1)
			m_ostr << "<any>";
		for (const auto& str : possibleNames)
		{
			const bool isMultiterm = str.find("</t><t>") != std::string::npos;
			if (isMultiterm)
				m_ostr << "<list>";
			m_ostr << str;
			if (isMultiterm)
				m_ostr << "</list>";
		}
		if (possibleNames.size() > 1)
			m_ostr << "</any>";
		m_ostr << "</pat>" << std::endl << "\t\t<tran>";
		m_ostr << "<mkent n='osmgen-" << m_entId << "' i='osmgen-" << m_entId << "' c='0' s='0' p='" << m_lat << ", " << m_lon << "' ";
		m_ostr << "/></tran>\n\t</stmt>" << std::endl;
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

Parser::Parser(std::ostream& ostr, size_t& entId)
: m_expat(XML_ParserCreate(NULL))
, m_state(State::AwaitingNode)
, m_ostr(ostr)
, m_numEntities(0)
, m_entId (entId)
{
	XML_SetUserData (m_expat, this);
	XML_SetElementHandler (m_expat, startElement, endElement);
	m_ostr.precision(8);
}

int main (int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "Usage: " << argv [0] << " outfile.xml map1.osm [map2.osm ...]" << std::endl;
		return 1;
	}

	std::ofstream ostr(argv[1]);

	ostr << "<?xml version='1.0' encoding='UTF-8'?>" << std::endl;
	ostr << "<stmset xmlns:xsi='http://www.w3.org/2000/10/XMLSchema-instance' xmlns='http://www.barzer.net/barzel/0.1'>" << std::endl;

	/*
	for (const auto& pair : amenitySynonyms)
	{
		ostr << "\t<stmt n='" << ++entId << "'>\n\t\t<pat>";
		if (pair.second.size() > 1)
			ostr << "<any>";
		for (const auto& syn : pair.second)
		{
			const bool isMultiterm = syn.find("</t><t>") != std::string::npos;
			if (isMultiterm)
				ostr << "<list>";
			ostr << "<t>" + syn + "</t>";
			if (isMultiterm)
				ostr << "</list>";
		}
		if (pair.second.size() > 1)
			ostr << "</any>";
		ostr << "</pat>\n\t\t<tran><t>" << pair.first << "</t></tran>";
		ostr << "\n\t</stmt>\n";
	}
	*/

	size_t entId = 0;

	for (int i = 2; i < argc; ++i)
	{
		std::cout << "parsing " << argv[i] << "..." << std::endl;
		std::ifstream istr(argv[i]);
		Parser p(ostr, entId);
		p.parse(istr);
		p.dumpStats();
	}

	ostr << "</stmset>" << std::endl;
}
