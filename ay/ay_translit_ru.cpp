#include "ay_translit_ru.h"

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
			RULE1("ц", 'f');
			RULE1("ь", '\'');
			RULE1("э", 'e');

			russian.push_back(*src++);
		}
	}
	*/

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

	void en2ru(const char *src, size_t len, std::string& russian)
	{
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
				switch (c1)
				{
				case 'e':
				case 'y':
					russian.append("э");
					++src;
					break;
				default:
					russian.append("а");
					break;
				}
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
			}
			case 'c':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("ц");
					break;
				}
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
			}
			case 'g':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("г");
					break;
				}
			}
			case 'h':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("х");
					break;
				}
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
			}
			case 'o':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("о");
					break;
				}
			}
			case 'p':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("п");
					break;
				}
			}
			case 'q':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("к");
					break;
				}
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
			}
			case 's':
			{
				char c1 = src + 1 < end ? src[1] : 0;
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
			}
			case 't':
			{
				char c1 = src + 1 < end ? src[1] : 0;
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
				switch (c1)
				{
				default:
					russian.append("у");
					break;
				}
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
			}
			case 'y':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				default:
					russian.append("ы");
					break;
				}
			}
			case 'z':
			{
				char c1 = src + 1 < end ? src[1] : 0;
				switch (c1)
				{
				case 'h':
					russian.append("ж");
					++src;
				default:
					russian.append("з");
					break;
				}
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
