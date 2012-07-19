#include "ay_translit_ru.h"
#include "ay_utf8.h"
#include <iostream>

namespace ay
{
namespace tl
{
#define CCMP(a,b) (a == b || a == b + 32)

#define RULE1(res,s1) \
	if (CCMP(src[0], s1)) \
	{ \
		russian.append(res); \
		++src; \
		continue; \
	}

#define RULE2(res,s1,s2) \
	if (src + 1 < end && \
		CCMP(src[0], s1) && \
		CCMP(src[1], s2)) \
	{ \
		russian.append(res); \
		src += 2; \
		continue; \
	}

#define RULE4(res,s1,s2,s3,s4) \
	if (src + 3 < end && \
		CCMP(src[0], s1) && \
		CCMP(src[1], s2) && \
		CCMP(src[2], s3) && \
		CCMP(src[3], s4)) \
	{ \
		russian.append(res); \
		src += 4; \
		continue; \
	}

	/*
	void en2ru (const char *src, size_t len, std::string& russian)
	{
		const char * const end = src + len;
		while (src < end)
		{
			RULE4("щ", 's', 'h', 'c', 'h');
			RULE2("ё", 'j', 'o');
			RULE2("ж", 'z', 'h');
			RULE2("х", 'k', 'h');
			RULE2("ч", 'c', 'h');
			RULE2("ш", 's', 'h');
			RULE2("ъ", 'y', 'h');
			RULE2("ы", 'e', 'e');
			RULE2("ю", 'j', 'u');
			RULE2("я", 'j', 'a');
			RULE1("а", 'a');
			RULE1("б", 'b');
			RULE1("в", 'v');
			RULE1("г", 'g');
			RULE1("д", 'd');
			RULE1("е", 'e');
			RULE1("з", 'z');
			RULE1("и", 'i');
			RULE1("й", 'y');
			RULE1("к", 'k');
			RULE1("л", 'l');
			RULE1("м", 'm');
			RULE1("н", 'n');
			RULE1("о", 'o');
			RULE1("п", 'p');
			RULE1("р", 'r');
			RULE1("с", 's');
			RULE1("т", 't');
			RULE1("у", 'u');
			RULE1("ф", 'f');
			RULE1("ц", 'c');
			RULE1("ь", '\'');
			RULE1("э", 'e');

			russian.push_back(*src++);
		}
	}
	*/

    namespace {
        struct TLException_t_compare_less {
            bool operator()( const TLException_t& x, const std::string& s ) const 
            {
                return( x.first< s );
            }
        };
    }
	void ru2en (const char *src, size_t len, std::string& english, const TLExceptionList_t& exceptions)
	{
		if (!exceptions.empty())
		{
			const std::string srcStr(src,len);

			auto pos = std::lower_bound( exceptions.begin(), exceptions.end(), srcStr, TLException_t_compare_less() );

			if (pos != exceptions.end() && pos->first == srcStr)
			{
				english = pos->second;
				return;
			}
		}

		const StrUTF8 russian(src, len);
		for (size_t i = 0; i < russian.size(); ++i)
		{
			const auto& c = russian[i];

			if (c == CharUTF8("a"))
				english.append("a");
			else if (c == CharUTF8("б"))
				english.append("b");
			else if (c == CharUTF8("в"))
				english.append("v");
			else if (c == CharUTF8("г"))
				english.append("g");
			else if (c == CharUTF8("д"))
				english.append("d");
			else if (c == CharUTF8("е"))
				english.append("e");
			else if (c == CharUTF8("ё"))
				english.append("jo");
			else if (c == CharUTF8("ж"))
				english.append("zh");
			else if (c == CharUTF8("з"))
				english.append("z");
			else if (c == CharUTF8("и"))
				english.append("i");
			else if (c == CharUTF8("й"))
				english.append("y");
			else if (c == CharUTF8("к"))
				english.append("k");
			else if (c == CharUTF8("л"))
				english.append("l");
			else if (c == CharUTF8("м"))
				english.append("m");
			else if (c == CharUTF8("н"))
				english.append("n");
			else if (c == CharUTF8("о"))
				english.append("o");
			else if (c == CharUTF8("п"))
				english.append("p");
			else if (c == CharUTF8("р"))
				english.append("r");
			else if (c == CharUTF8("с"))
				english.append("s");
			else if (c == CharUTF8("т"))
				english.append("t");
			else if (c == CharUTF8("у"))
				english.append("u");
			else if (c == CharUTF8("ф"))
				english.append("f");
			else if (c == CharUTF8("х"))
				english.append("kh");
			else if (c == CharUTF8("ц"))
				english.append("c");
			else if (c == CharUTF8("ч"))
				english.append("ch");
			else if (c == CharUTF8("ш"))
				english.append("sh");
			else if (c == CharUTF8("щ"))
				english.append("sch");
			else if (c == CharUTF8("ъ"))
				english.append("yh");
			else if (c == CharUTF8("ы"))
				english.append("ee");
			else if (c == CharUTF8("ь"))
				english.append("'");
			else if (c == CharUTF8("э"))
				english.append("a");
			else if (c == CharUTF8("ю"))
				english.append("ju");
			else if (c == CharUTF8("я"))
				english.append("ya");
			else
				english.append(c);
		}
	}

	namespace
	{
		inline bool isVowel(char c)
		{
			switch (c)
			{
			case 'e':
			case 'y':
			case 'u':
			case 'i':
			case 'o':
			case 'a':
				return true;
			default:
				return false;
			}
		}
	}

	void en2ru(const char *src, size_t len, std::string& russian, const TLExceptionList_t& exceptions)
	{
		if (!exceptions.empty())
		{
			const std::string srcStr(src);

			auto pos = std::lower_bound(exceptions.begin(), exceptions.end(), srcStr, TLException_t_compare_less() );
			if (pos != exceptions.end() && pos->first == srcStr)
			{
				russian = pos->second;
				return;
			}
		}

		const char * const end = src + len;
		char prev = 0;
		while (src < end)
		{
			char c0 = src[0];
			switch (c0)
			{
			case 'a':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				if (!c1 || !isVowel(c1))
				{
					russian.append("а");
					break;
				}

				switch (c1)
				{
				case 'e':
				case 'y':
					russian.append("э");
					++src;
					break;
				default:
					russian.append("э");
					break;
				}

				break;
			}
			case 'b':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("б");
					break;
				}

				break;
			}
			case 'c':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'h':
					russian.append("ч");
					break;
				default:
					russian.append("ц");
					break;
				}

				break;
			}
			case 'd':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("д");
					break;
				}

				break;
			}
			case 'e':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'e':
				case 'a':
					russian.append("и");
					++src;
					break;
				case 'y':
					russian.append("ей");
					++src;
					break;
				default:
					russian.append("е");
					break;
				}

				break;
			}
			case 'f':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("ф");
					break;
				}

				break;
			}
			case 'g':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'h':
					++src;
				default:
					russian.append("г");
					break;
				}

				break;
			}
			case 'h':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				if (!c1 || !isVowel(c1))
					break;

				switch (c1)
				{
				default:
					russian.append("х");
					break;
				}

				break;
			}
			case 'i':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("и");
					break;
				}

				break;
			}
			case 'j':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("ж");
					break;
				}

				break;
			}
			case 'k':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'n':
					russian.append("n");
					++src;
					break;
				default:
					russian.append("к");
					break;
				}

				break;
			}
			case 'l':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("л");
					break;
				}

				break;
			}
			case 'm':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("м");
					break;
				}

				break;
			}
			case 'n':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("н");
					break;
				}

				break;
			}
			case 'o':
			{
				char c1 = src + 1 < end ? src[1] : 0;

				if (src + 4 <= end &&
					c1 == 'u' &&
					src[2] == 'g' &&
					src[3] == 'h')
				{
					if (src + 4 < end)
						russian.append("о");
					else
						russian.append("ф");
					src += 3;
					break;
				}

				switch (c1)
				{
				default:
					russian.append("о");
					break;
				}

				break;
			}
			case 'p':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'h':
					russian.append("ф");
				default:
					russian.append("п");
					break;
				}

				break;
			}
			case 'q':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'u':
					russian.append("кв");
					++src;
					break;
				default:
					russian.append("к");
					break;
				}

				break;
			}
			case 'r':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("р");
					break;
				}

				break;
			}
			case 's':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				if (src + 2 < end && c1 == 'c' && src[2] == 'h')
				{
					russian.append("щ");
					src += 2;
					break;
				}
				if (isVowel(prev) && isVowel(c1))
				{
					russian.append("z");
					break;
				}

				switch (c1)
				{
				case 'h':
					russian.append("ш");
					++src;
					break;
				default:
					russian.append("с");
					break;
				}

				break;
			}
			case 't':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				if (src + 2 < end && c1 == 'c' && src[2] == 'h')
				{
					russian.append("ч");
					src += 2;
					break;
				}

				switch (c1)
				{
				case 'z':
				case 's':
					russian.append("ц");
					++src;
					break;
				case 'h':
					if (src + 2 < end && isVowel(src[2]))
						russian.append("з");
					else
						russian.append("т");
					++src;
					break;
				default:
					russian.append("т");
					break;
				}

				break;
			}
			case 'u':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				if (c1 && !isVowel(c1))
				{
					russian.append("а");
					break;
				}

				switch (c1)
				{
				default:
					russian.append("у");
					break;
				}

				break;
			}
			case 'v':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("в");
					break;
				}

				break;
			}
			case 'w':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("уа");
					break;
				}

				break;
			}
			case 'x':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("кс");
					break;
				}

				break;
			}
			case 'y':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				if (prev && !isVowel(prev))
				{
					if (c1 == 'a')
					{
						russian.append("ья");
						++src;
						break;
					}
					else if (c1 == 'e')
					{
						russian.append("ье");
						++src;
						break;
					}
				}

				switch (c1)
				{
				default:
					russian.append("ы");
					break;
				}

				break;
			}
			case 'z':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'z':
					russian.append("ц");
					++src;
					break;
				case 'h':
					russian.append("ж");
					++src;
					break;
				default:
					russian.append("з");
					break;
				}

				break;
			}
			default:
				russian.push_back(c0);
				break;
			}

			prev = c0;
			++src;
		}
	}
}
}
