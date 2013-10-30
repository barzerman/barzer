#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/tokenizer.hpp>
#include <boost/concept_check.hpp>
#include <boost/algorithm/string/join.hpp>
#include <libstemmer.h>

const size_t GramSize = 3;
const size_t MaxLength = 2;
const size_t PopThreshold = 1;

const std::vector<std::string> Markers = { "a", "the" };
const std::vector<std::string> EndShit = { "a", "the", "in", "on", "to", "from", "of", "for", "until", "and", "or", "but", "of", "if", "i", "we", "us", "you" };
const std::vector<std::string> BothShit = { "and", "or", "but", "of", "if", "i", "we", "us", "you" };

namespace
{
	std::string Stem (const std::string& s)
	{
		std::shared_ptr<sb_stemmer> stemmer { sb_stemmer_new ("english", nullptr), sb_stemmer_delete };
		return reinterpret_cast<const char*> (sb_stemmer_stem (stemmer.get (),
						reinterpret_cast<const unsigned char*> (s.c_str ()),
						s.size ()));
	}
}

struct LinkedText
{
	std::string Stemmed_;
	std::string OrigText_;

	LinkedText (const std::string& s)
	: Stemmed_ { Stem (s) }
	, OrigText_ { s }
	{
	}
};

bool operator== (const LinkedText& l1, const LinkedText& l2)
{
	return l1.Stemmed_ == l2.Stemmed_;
}

namespace std
{
	template<>
	struct hash<LinkedText>
	{
		size_t operator() (const LinkedText& l) const
		{
			return hash<std::string> {} (l.Stemmed_);
		}
	};

	template<>
	struct hash<std::vector<LinkedText>>
	{
		size_t operator() (const std::vector<LinkedText>& l) const
		{
			const hash<LinkedText> h {};
			return std::accumulate (l.begin (), l.end (), 0,
						[&h] (size_t res, const LinkedText& s) { return res + h (s); });
		}
	};
}

typedef std::unordered_map<std::string, std::string> FileMap_t;
typedef std::unordered_map<std::string, std::vector<std::vector<LinkedText>>> GramsMap_t;
typedef std::unordered_map<std::vector<LinkedText>, std::vector<std::string>> RevMap_t;
typedef std::unordered_map<std::vector<LinkedText>, size_t> RevStatMap_t;

FileMap_t ReadFile (const std::string& fname)
{
	FileMap_t map;
	std::ifstream istr { fname };
	while (true)
	{
		std::string str;
		std::getline (istr, str);
		if (str.empty ())
			break;

		const auto pos = str.find ("|");
		const auto& docName = str.substr (0, pos);
		const auto& text = str.substr (pos + 1);

		const auto mpos = map.find (docName);
		if (mpos == map.end ())
			map [docName] = text;
		else
			mpos->second += " " + text;
	}
	return map;
}

template<typename T>
GramsMap_t::mapped_type GetGrams (const std::vector<T>& words)
{
	GramsMap_t::mapped_type grams;

	const auto wc = words.size ();

	for (size_t gs = 1, maxGs = std::min (GramSize, words.size () - 1); gs <= maxGs; ++gs)
		for (size_t i = 0; i < wc - gs + 1; ++i)
			grams.push_back ({ words.begin () + i, words.begin () + i + gs });

	return std::move (grams);
}

GramsMap_t Grammize (const FileMap_t& map)
{
	GramsMap_t result;

	for (const auto& pair : map)
	{
		std::vector<std::string> words;

		boost::char_separator<char> sep { ",.!? " };
		boost::tokenizer<boost::char_separator<char>> tokens { pair.second, sep };
		for (const auto& t : tokens)
			words.push_back ({ t });

		result [pair.first] = GetGrams (words);
	}

	return result;
}

RevMap_t Invert (const GramsMap_t& map)
{
	RevMap_t result;
	for (const auto& pair : map)
		for (const auto& gram : pair.second)
			result [gram].push_back (pair.first);
	return result;
}

RevStatMap_t ToStats (const RevMap_t& map)
{
	RevStatMap_t result;
	for (const auto& pair : map)
		result.insert ({ pair.first, pair.second.size () });
	return result;
}

RevStatMap_t Reduce (RevStatMap_t map)
{
	for (auto i = map.begin (); i != map.end (); )
	{
		bool iUpdated = false;
		for (const auto& gram : GetGrams (i->first))
		{
			if (gram == i->first)
				continue;

			const auto pos = map.find (gram);
			if (pos != map.end () && pos->second == i->second)
			{
				map.erase (pos);
				iUpdated = true;
			}
		}

		if (!iUpdated)
			++i;
	}

	return map;
}

namespace
{
	bool Contains (const std::vector<std::string>& words, const LinkedText& t)
	{
		return std::any_of (words.begin (), words.end (),
				[&t] (const std::string& w) { return t.Stemmed_ == w; });
	}
}

RevStatMap_t FilterShit (RevStatMap_t map)
{
	for (auto i = map.begin (); i != map.end (); )
	{
		const auto& gram = i->first;

		if (Contains (EndShit, *gram.rbegin ()) ||
				std::all_of (gram.begin (), gram.end (),
						[] (const LinkedText& t) { return Contains (BothShit, t); }))
			i = map.erase (i);
		else
			++i;
	}
	return map;
}

RevStatMap_t FilterPop (RevStatMap_t map)
{
	for (auto i = map.begin (); i != map.end (); )
	{
		if (i->second <= PopThreshold)
			++i;
		else
			i = map.erase (i);
	}
	return map;
}

RevStatMap_t FilterLength (RevStatMap_t map)
{
	for (auto i = map.begin (); i != map.end (); )
	{
		const auto& gram = i->first;

		size_t count = 0;
		for (const auto& w : gram)
			if (!Contains (Markers, w))
				++count;

		if (count > MaxLength)
			i = map.erase (i);
		else
			++i;
	}
	return map;
}

void Print (const RevStatMap_t& map)
{
	std::vector<std::string> texts;
	for (const auto& pair : map)
	{
		std::vector<std::string> origs;
		for (const auto& t : pair.first)
		{
			if (Contains (Markers, t.OrigText_))
				origs.push_back ("opt(" + t.OrigText_ + ")");
			else
				origs.push_back (t.OrigText_);
		}

		texts.push_back (boost::algorithm::join (origs, " "));
	}

	std::sort (texts.begin (), texts.end ());
	for (const auto& str : texts)
		std::cout << str << std::endl;
}

template<typename T>
T Id (const T& t)
{
	return t;
}

int main (int argc, char **argv)
{
	if (argc < 2)
		std::cerr << "Usage: " << argv [0] << " datafile.txt" << std::endl;

	Print (Reduce (FilterLength (FilterShit (FilterPop (ToStats (Invert (Grammize (ReadFile (argv [1])))))))));

	return 0;
}
