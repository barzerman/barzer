#include "ay_translit_ru.h"
#include "ay_utf8.h"
#include "ay_util.h"
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

namespace {

#define char_cp_Pair(from,to) std::pair< const char* , const char* >( from, to )

typedef std::pair< const char* , const char* > char_cp_pair;

char_cp_pair g_enRuMap[] = {
char_cp_pair("a","э"),
char_cp_pair("able","эйбл"),
char_cp_pair("about","эбаут"),
char_cp_pair("above","эбав"),
char_cp_pair("after","афтер"),
char_cp_pair("all","олл"),
char_cp_pair("also","олсо"),
char_cp_pair("an","эн"),
char_cp_pair("and","энд"),
char_cp_pair("any","эни"),
char_cp_pair("as","аз"),
char_cp_pair("ask","аск"),
char_cp_pair("at","эт"),
char_cp_pair("back","бэк"),
char_cp_pair("bad","бэд"),
char_cp_pair("be","би"),
char_cp_pair("because","бекоз"),
char_cp_pair("beneath","бенис"),
char_cp_pair("big","биг"),
char_cp_pair("but","бат"),
char_cp_pair("by","бай"),
char_cp_pair("call","кол"),
char_cp_pair("can","кэн"),
char_cp_pair("case","кейс"),
char_cp_pair("child","чайлд"),
char_cp_pair("come","кам"),
char_cp_pair("company","компани"),
char_cp_pair("could","куд"),
char_cp_pair("day","дей"),
char_cp_pair("different","дифферент"),
char_cp_pair("do","ду"),
char_cp_pair("early","эрли"),
char_cp_pair("even","ивен"),
char_cp_pair("eye","ай"),
char_cp_pair("fact","факт"),
char_cp_pair("feel","фил"),
char_cp_pair("few","фью"),
char_cp_pair("find","файнд"),
char_cp_pair("first","ферст"),
char_cp_pair("for","фо"),
char_cp_pair("from","фром"),
char_cp_pair("get","гет"),
char_cp_pair("give","гив"),
char_cp_pair("go","гоу"),
char_cp_pair("good","гуд"),
char_cp_pair("government","гавермент"),
char_cp_pair("great","грейт"),
char_cp_pair("group","груп"),
char_cp_pair("hand","хэнд"),
char_cp_pair("have","хэв"),
char_cp_pair("he","хи"),
char_cp_pair("her","хё"),
char_cp_pair("here","хиа"),
char_cp_pair("high","хай"),
char_cp_pair("him","хим"),
char_cp_pair("his","хиз"),
char_cp_pair("how","хау"),
char_cp_pair("i","ай"),
char_cp_pair("if","иф"),
char_cp_pair("in","ин"),
char_cp_pair("into","инту"),
char_cp_pair("it","ит"),
char_cp_pair("its","итс"),
char_cp_pair("know","ноу"),
char_cp_pair("large","лардж"),
char_cp_pair("last","ласт"),
char_cp_pair("leave","лив"),
char_cp_pair("life","лайф"),
char_cp_pair("like","лайк"),
char_cp_pair("little","литл"),
char_cp_pair("long","лонг"),
char_cp_pair("look","лук"),
char_cp_pair("make","мейк"),
char_cp_pair("man","мэн"),
char_cp_pair("me","ми"),
char_cp_pair("most","мост"),
char_cp_pair("my","май"),
char_cp_pair("new","нью"),
char_cp_pair("next","некст"),
char_cp_pair("no","но"),
char_cp_pair("not","нот"),
char_cp_pair("now","нау"),
char_cp_pair("number","нау"),
char_cp_pair("of","оф"),
char_cp_pair("old","олд"),
char_cp_pair("on","он"),
char_cp_pair("one","уан"),
char_cp_pair("only","онли"),
char_cp_pair("or","о"),
char_cp_pair("other","азе"),
char_cp_pair("our","ауа"),
char_cp_pair("out","аут"),
char_cp_pair("over","оувер"),
char_cp_pair("own","оун"),
char_cp_pair("pear","пеар"),
char_cp_pair("people","пипл"),
char_cp_pair("person","пёрсон"),
char_cp_pair("place","плейс"),
char_cp_pair("point","пойнт"),
char_cp_pair("problem",""),
char_cp_pair("public","паблик"),
char_cp_pair("right","райт"),
char_cp_pair("sake","саке"),
char_cp_pair("same","сэйм"),
char_cp_pair("say","сэй"),
char_cp_pair("see","си"),
char_cp_pair("seem","сим"),
char_cp_pair("she","ши"),
char_cp_pair("small","смол"),
char_cp_pair("so","со"),
char_cp_pair("some","сам"),
char_cp_pair("take","тэйк"),
char_cp_pair("tell","тэлл"),
char_cp_pair("than","зэн"),
char_cp_pair("that","зэт"),
char_cp_pair("the","зэ"),
char_cp_pair("their","зэир"),
char_cp_pair("them","зэм"),
char_cp_pair("then","зэн"),
char_cp_pair("there","зэе"),
char_cp_pair("these","зиз"),
char_cp_pair("they","зэй"),
char_cp_pair("thing","сынг"),
char_cp_pair("think","сынк"),
char_cp_pair("this","зыс"),
char_cp_pair("time","тайм"),
char_cp_pair("to","ту"),
char_cp_pair("try","трай"),
char_cp_pair("two","ту"),
char_cp_pair("under","андер"),
char_cp_pair("up","ап"),
char_cp_pair("us","ас"),
char_cp_pair("use","юз"),
char_cp_pair("want","уонт"),
char_cp_pair("way","уэй"),
char_cp_pair("we","уи"),
char_cp_pair("week","уик"),
char_cp_pair("well","уэлл"),
char_cp_pair("what","уот"),
char_cp_pair("when","уэн"),
char_cp_pair("which","уич"),
char_cp_pair("who","ху"),
char_cp_pair("will","уил"),
char_cp_pair("with","уиз"),
char_cp_pair("woman","вумэн"),
char_cp_pair("work","уорк"),
char_cp_pair("world","уорлд"),
char_cp_pair("would","вуд"),
char_cp_pair("year","еар"),
char_cp_pair("you","ю"),
char_cp_pair("young","янг"),
char_cp_pair("your","ёр")
};
struct PAIR_compare_less {
    bool operator() ( const char_cp_pair& p, const std::string& s ) const
        { return (strcmp(p.first,s.c_str()) < 0); }
    bool operator() ( const std::string& s, const char_cp_pair& p ) const
        { return (strcmp(s.c_str(),p.first) < 0); }
};

const char* enRuHarcodedLookup( const std::string& s ) {
    auto i = std::lower_bound( ARR_BEGIN(g_enRuMap), ARR_END(g_enRuMap), s, TLException_t_compare_less() );
    return( i == ARR_END(g_enRuMap) ? 0 : (s==i->first ? i->second:0) );
}

const char* g_singleEnCharTranslit[] = {
"а" , // a
"б" , // b
"к" , // c
"д" , // d
"е" , // e
"ф" , // f
"г" , // g
"х" , // h
"и" , // i
"дж" , // j
"к" , // k
"л" , // l
"м" , // m
"н" , // n
"о" , // o
"р" , // p
"к" , // q
"р" , // r
"с" , // s
"т" , // t
"у" , // u
"в" , // v
"у" , // w
"кс" , // x
"ы" , // y
"з"  // z
};
const char* getSingleEnCharTranslit( char c ) 
{
    return ( c>='a' && c<= 'z' ? g_singleEnCharTranslit[c-'a']:"") ;
}

}

	void en2ru(const char *tmp, size_t len, std::string& russian, const TLExceptionList_t& exceptions)
	{
        std::string srcStr(tmp,len);
        const char* trans = enRuHarcodedLookup(srcStr);
        if( trans ) {
            russian=trans;
            return ;
        }
        const char* src = srcStr.c_str();
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

		const char * const beg = src;
		const char * const end = src + len;
		char prev = 0;
		while (src < end)
		{
            const char* src_1 = src+1;
			char c0 = src[0];
		    char c1 = src_1 < end ? src[1] : 0;
            const char * c0default = getSingleEnCharTranslit(c0);
            char c_prev = ( src> beg ? *(src-1) : 0 );

			switch (c0)
			{
			case 'a':
			{
                if( c1 =='c' && src[2] == 'k' ) {
                    russian.append("э");
                } else if (!c1 || !isVowel(c1)) {
					russian.append( c_prev == 'i' ? "я" : "а");
				} else { 
                    switch (c1) {
                    case 'e':
                    case 'y':
                        russian.append("э");
                        ++src;
                        break;
                    default:
                        russian.append("э");
                        break;
                    }
                }

				break;
			}
			case 'b':
			{
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
				switch (c1)
				{
				case 'k': russian.append("к"); ++src; break;
				case 'h': russian.append("ч"); ++src; break;
				case 'i':
				case 'e': russian.append("ц"); break;
				default:
					russian.append(c0default);
					break;
				}

				break;
			}
			case 'd':
			{
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
                if( c1 == 'a' && src[2] == 'r' && !src[3] ) {
                    russian.append("иа");
                    src+=3;
                } else {
                    
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
				break;
			}
			case 'f':
			{
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
				switch (c1)
				{
                case 'e':
                    if( src[2] == 'n' && src[3] == 'd' && c_prev == 'r' ) {
                        ++src;
                        russian.append("е");
                    } else 
                        russian.append("и");
                    break;
				default:
					russian.append("и");
					break;
				}

				break;
			}
			case 'j':
			{
                if( src_1 == end ) {
					russian.append("дж");
                } else {
				    switch (c1)
				    {
                    case 'j': russian.append("дж"); src+=1; break;
                    case 'u': russian.append("ю"); src+=1; break;
                    case 'e': russian.append("ж"); break;
				    default:
					    russian.append(c0default);
					    break;
				    }
                }

				break;
			}
			case 'k':
			{
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

				if (src + 4 <= end && c1 == 'u' && src[3] == 'e' ) { // ouXe[$]
                    russian.append( "ау" ) ;
                    russian.append( getSingleEnCharTranslit( src[2] ) );
                    src+= 3;
                    break;
				} else if (src + 4 <= end && c1 == 'u' && src[2] == 'g' && src[3] == 'h') {
					if (src + 4 < end)
						russian.append("о");
					else
						russian.append("ф");
					src += 3;
					break;
				} else if( src +2 <= end ) {
                    if( c1 == 'o' ) russian.append( "у" ) ;
                    else if( c1 == 'u' ) russian.append( "ау" ) ;
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
				case 'h': russian.append("ш"); ++src; break;
				default:
					russian.append("с");
					break;
				}

				break;
			}
			case 't':
			{
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
				if (c1 && !isVowel(c1) && !isVowel(src[2]) )
				{
                    if( c_prev && strchr( "bdtprfm", c_prev ) )
					    russian.append( c1 == 's' ? "у": "а");
                    else 
                        russian.append("у");
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

					russian.append( src_1 == end ? "и" : "ы");
					break;
				}

				break;
			}
			case 'z':
			{
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
