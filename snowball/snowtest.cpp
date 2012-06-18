#include <fstream>
#include <iostream>
#include <vector>
#include <sys/time.h>

#include "libstemmer_c/include/libstemmer.h"

inline long getDiff (timeval prev, timeval now)
{
	return 1000000 * (now.tv_sec - prev.tv_sec) + now.tv_usec - prev.tv_usec;
}

inline std::string stem (const std::string& word, sb_stemmer *stemmer)
{
	const sb_symbol *pstr = reinterpret_cast<const sb_symbol*> (word.c_str ());
	const char *result = reinterpret_cast<const char*> (sb_stemmer_stem (stemmer, pstr, word.size ()));
	return std::string (result, sb_stemmer_length (stemmer));
}

int main (int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv [0] << " datafile" << std::endl;
		return 1;
	}
	std::ifstream istr (argv [1]);
	std::vector<std::string> strings;
	while (istr.good ())
	{
		std::string str;
		istr >> str;

		if (str.size () < 2)
			continue;
		strings.push_back (str);
	}
	std::cout << "read " << strings.size () << " words" << std::endl;

	timeval prev, now;
	gettimeofday (&prev, 0);
	sb_stemmer *stemmer = sb_stemmer_new ("en", 0);
	gettimeofday (&now, 0);
	std::cout << "stemmer creation took " << getDiff (prev, now) << " μs" << std::endl;

	gettimeofday (&prev, 0);
	for (int i = 0, size = strings.size (); i < size; ++i)
		stem (strings.at (i), stemmer);
	gettimeofday (&now, 0);
	std::cout << "stemming took " << getDiff (prev, now) << " μs" << std::endl;

	gettimeofday (&prev, 0);
	sb_stemmer_delete (stemmer);
	gettimeofday (&now, 0);
	std::cout << "stemmer deletion took " << getDiff (prev, now) << " μs" << std::endl;
}
