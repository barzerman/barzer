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
	if (CCMP(s[0], s1)) \
	{ \
		russian.append(res); \
		++s; \
		continue; \
	}

#define RULE2(res,s1,s2) \
	if (s + 1 < end && \
		CCMP(s[0], s1) && \
		CCMP(s[1], s2)) \
	{ \
		russian.append(res); \
		s += 2; \
		continue; \
	}

#define RULE4(res,s1,s2,s3,s4) \
	if (s + 3 < end && \
		CCMP(s[0], s1) && \
		CCMP(s[1], s2) && \
		CCMP(s[2], s3) && \
		CCMP(s[3], s4)) \
	{ \
		russian.append(res); \
		s += 4; \
		continue; \
	}

	/*
	void en2ru (const char *s, size_t len, std::string& russian)
	{
		const char * const end = s + len;
		while (s < end)
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

			russian.push_back(*s++);
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
	void ru2en (const char *s, size_t len, std::string& english, const TLExceptionList_t& exceptions)
	{
		if (!exceptions.empty())
		{
			const std::string srcStr(s,len);

			auto pos = std::lower_bound( exceptions.begin(), exceptions.end(), srcStr, TLException_t_compare_less() );

			if (pos != exceptions.end() && pos->first == srcStr)
			{
				english = pos->second;
				return;
			}
		}

		const StrUTF8 russian(s, len);
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
		inline bool isNonVowel(char c)
            { return ( c && !isVowel(c)); }
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
char_cp_pair("anyone","эниван"),
char_cp_pair("anyones","эниванс"),
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
char_cp_pair("cent","цент"),
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
char_cp_pair("given","гивен"),
char_cp_pair("go","гоу"),
char_cp_pair("good","гуд"),
char_cp_pair("government","гавермент"),
char_cp_pair("great","грейт"),
char_cp_pair("group","груп"),
char_cp_pair("hand","хэнд"),
char_cp_pair("has","хэз"),
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
char_cp_pair("is","из"),
char_cp_pair("isn't","изнт"),
char_cp_pair("it","ит"),
char_cp_pair("its","итс"),
char_cp_pair("june","джун"),
char_cp_pair("july","джулай"),
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
char_cp_pair("same","сэйм"),
char_cp_pair("say","сэй"),
char_cp_pair("see","си"),
char_cp_pair("seem","сим"),
char_cp_pair("she","ши"),
char_cp_pair("should","шуд"),
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
char_cp_pair("today","тудей"),
char_cp_pair("try","трай"),
char_cp_pair("two","ту"),
char_cp_pair("under","андер"),
char_cp_pair("up","ап"),
char_cp_pair("us","ас"),
char_cp_pair("use","юз"),
char_cp_pair("want","уонт"),
char_cp_pair("was","воз"),
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
inline const char* getSingleEnCharTranslit( char c ) 
{
    return ( c>='a' && c<= 'z' ? g_singleEnCharTranslit[c-'a']:"") ;
}

inline bool terminating_char( char c ) { return ( !c || !(c>='a'&&c<='z') ); }

} /// anonymous namespace

	void en2ru(const char *tmp, size_t len, std::string& russian, const TLExceptionList_t& exceptions)
	{
        std::string srcStr(tmp,len);
        const char* trans = enRuHarcodedLookup(srcStr);
        if( trans ) {
            russian=trans;
            return ;
        }
        const char* s = srcStr.c_str();
		if (!exceptions.empty())
		{
			const std::string srcStr(s);

			auto pos = std::lower_bound(exceptions.begin(), exceptions.end(), srcStr, TLException_t_compare_less() );
			if (pos != exceptions.end() && pos->first == srcStr)
			{
				russian = pos->second;
				return;
			}
		}

		const char * const beg = s;
		const char * const end = s + len;
		char prev = 0;
		while (s < end)
		{
            const char* src_1 = s+1;
			char c0 = s[0];
		    char c1 = src_1 < end ? s[1] : 0;
            const char * c0default = getSingleEnCharTranslit(c0);
            char c_prev = ( s> beg ? *(s-1) : 0 ), c_prev_1 = (s>beg+1? *(s-2) : 0 );

			switch (c0)
			{
			case 'a':
			{
                if( s[1] == 't' && s[2] == 'i' && s[3] =='o' && s[4] =='n' ) {
                    russian.append("ейшен");
                    s+=4;
                } else if( s[1] && !isVowel(s[1]) && s[2] =='i' && s[3]=='n' && s[4] =='g' && terminating_char(s[5]) ) {
                    russian.append("ей");
                } else if( s[1] == 'n' && s[2] =='g' && (s[3] =='e'||s[3]=='i') ) {
                    russian.append("эй"); 
                } else if( s[1] == 'l' && s[2] == 'l' && s[3] =='e' && s[4] =='d' ) {
                    russian.append("олд");
                    s+=4;
                } else if( s[1] =='w' && s[2] =='l' ) {
                    russian.append((s+=1,"оу"));
                } else if( s[1] =='r' && s[2] =='e' && terminating_char(s[3]) ) {
                    russian.append((s+=2,"эйр"));
                } else if( s[1] == 'l' && s[2]=='l' ) {
                    russian.append("о");
                } else if( s[1] =='r' && !isVowel(s[2]) ) {
                    russian.append("а");
                } else if( s[1] && !isVowel(s[1]) && s[1] != 'r' && (s[2]=='e') ) {
                    russian.append(!strchr("cxhk",s[1])?"эй":"ей"); 
                } else if ( isNonVowel(c_prev) && s[1]=='l' &&s[2]=='k' ) {
                    russian.append("ок");
                    s+=2;
                } else if ( isNonVowel(c_prev) && s[1]=='l' && (s[2]=='t'||s[2]=='d') ) {
                    russian.append("о");
                } else if( s[1] && !isVowel(s[1]) && s[2] && !isVowel(s[2])) {
                    russian.append("э"); 
                } else if( s[1] == 'u'  ) {
                    russian.append((++s,"ау"));
                } else if( s[1] == 'l' && s[2] == 'l' ) {
                    russian.append("ол");
                    s+=2;
                } else if( c1 =='c' && s[2] == 'k' ) {
                    russian.append("э");
                } else if (!c1 || !isVowel(c1)) {
					russian.append( c_prev == 'i'&& terminating_char(s[1]) && beg+3<s ? "я" : "а");
				} else { 
                    switch (c1) {
                    case 'e':
                        russian.append("э");
                        ++s;
                        break;
                    case 'y':
                        russian.append("эй");
                        ++s;
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
                case 'r':  // br
                    if( s[2]=='e'&&s[3]=='a'&&s[4]=='k')  // break
                        {    russian.append("брэйк"); s+=4; }
                    else
                        russian.append("б");
                    break;
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
				case 'k': russian.append("к"); ++s; break;
				case 'h': russian.append("ч"); ++s; break;
				case 'i':
				case 'y':
				case 'e': russian.append("с"); break;
				default:
					russian.append(c0default);
					break;
				}

				break;
			}
			case 'd':
			{
                if( s[1] =='g' && s[2] =='e' ) {
                } else if( s[1] =='o' && s[2] =='e' && s[3] =='s'  ) {
                    russian.append((s+=3,"даз"));
                } else {
				switch (c1)
				{
				default:
					russian.append("д");
					break;
				}
                }

				break;
			}
			case 'e':
			{
               
                if( s[1] =='a' && s[2] == 't'&&s[3]=='h' && (strchr("wlnh",c_prev)) ) {
                    russian.append("эз");
                    s+=3;
                } else if( terminating_char(c_prev) && s[1] =='a' && s[2] == 'r'  ) {
                    russian.append("эа");
                    ++s;
                } else if( s[1] =='i' && s[2] == 'v'  ) {
                    russian.append("ив");
                    s+=2;
                } else if( s[1] =='i' && s[2]=='g' && s[3] =='h'  ) {
                    russian.append((s+=3,(c_prev =='h'? "ай":"ей") ));
                } else if( s[1] =='e' && isNonVowel(s[2]) && isVowel(s[3]) ) {
                    russian.append((s+=3,"из"));
                } else if( s[1] =='d' && s[2] == 'i' && s[3] == 't' ) {
                    russian.append("эдит");
                    s+=3;
                } else if( s[1] =='v' && s[2] == 'e' && s[3] == 'r' && s[4] == 'y' ) {
                    russian.append("эври");
                    s+=4;
                } else 
                if( terminating_char(s[1]) && !isVowel(c_prev) && (s-beg>2 || isVowel(*(s-2))) ) {
                } else if( c1 == 'a' && s[2] == 'r' && terminating_char(s[3]) ) {
                    russian.append("иа");
                    s+=3;
                } else if( s[1] =='w'  ) {
                    if( c_prev && c_prev != 'g' && !isVowel(c_prev) ) {
					    russian.append((++s,"ью"));
                    } else 
                        russian.append("е");
                } else {
                    
				    switch (c1)
				    {
				    case 'e':
				    case 'a':
					    russian.append("и");
					    ++s;
					    break;
				    case 'y':
					    russian.append("ей");
					    ++s;
					    break;
				     default:
					    russian.append(!c_prev ? "э": "е" );
					    break;
				    }
                }
				break;
			}
			case 'f':
			{
                if( s[1] == 'u' && s[2] =='t' && s[3] == 'u' ) {
                    russian.append((s+=3,"фьюче"));

                } else {
				switch (c1)
				{
				default:
					russian.append("ф");
					break;
				}
                }

				break;
			}
			case 'g':
			{
                if( s[1] =='i' && isVowel(s[2]) ) {
					russian.append((++s,"дж"));
                } else if( s[1] =='i' && isNonVowel(s[2])&&s[2]!='n' ) {
                    russian.append("г");
                } else if( s[1] =='u' && s[2]=='e' && (terminating_char(s[3])||(terminating_char(s[4])&&isNonVowel(s[3]))) ) {
                    russian.append((s+=2,"г"));
                } else if( s[1] =='i' || s[1]=='y' ) {
					russian.append((++s,"джи"));
                } else if( s[1] =='e' && (s[2] =='t' || s[3] =='h') ) {
					russian.append((s+=1,"ге"));
                } else if( s[1] =='e' ) {
					russian.append(isVowel(s[2]) ? "г":"дж");
                } else if( s[1] =='g' ) {
					russian.append((++s,"г"));
                } else {
				switch (c1)
				{
				case 'h':
					++s;
				default:
					russian.append("г");
					break;
				}
                }

				break;
			}
			case 'h':
			{
				if (!c1 || !isVowel(c1))
					break;

				switch (c1)
				{
                case 'e': // he
                    if( s[2]=='a' && s[3] == 'r' && s[4]=='t' ){
					    russian.append("харт");
                        s+=4;
                    }
                        
                    break;
				default:
					russian.append("х");
					break;
				}

				break;
			}
			case 'i':
			{
                if( s[1]=='n' && s[2] == 'i' && s[3]=='t' )  {
                    russian.append((s+=2,"ини"));
                } else if( s[1]=='e' && s[2] == 'd' )  {
                    russian.append((s+=1,"ай"));
                } else if( s[1]=='e' && s[2] == 'r' )  {
                    russian.append((s+=1,"ие"));
                } else if( s[1] == 'a' && isNonVowel(s[2]) )  {
                    russian.append((s+=1,"а"));
                } else if( s[1]=='v' && s[2] == 'e' && (terminating_char(s[3])||s[3]=='l'))  {
                    russian.append((s+=2,"ив"));
                } else if( (s[1]=='d'||s[1] =='t') && s[2] == 'i' && s[3] =='e')  {
                    russian.append("и");
                } else if( s[1] =='d' && s[2] == 'e' && s[3] =='n' && (s[4] =='c' || s[4] =='t') ) {
                    russian.append("и");
                } else if( s[1] && !isVowel(s[1]) && s[2] =='i' ) {
                    russian.append("и"); 
                } else if( !c_prev && s[1] == 'n'  ) {
                    russian.append("и"); 
                } else if( s[1] =='e' && s[2] =='s' && terminating_char(s[3])  ) {
                    russian.append("ис");
                    s+=2;
                } else if( s[1] =='e' ) {
                    russian.append((++s,"и"));
                } else
                if( s[1] && (s[2] =='e'||s[2] =='i') && !isVowel(s[1]) ) {
                    russian.append("ай");
                } else {
				    switch (c1)
				    {
                    case 'e':
                        if( s[2] == 'n' && s[3] == 'd' && c_prev == 'r' ) {
                            ++s;
                            russian.append("е");
                        } else 
                            russian.append("и");
                        break;
                    case 'g': 
                        if( s[2] == 'h' ) {
                            russian.append((++s,"ай"));
                        } else
                            russian.append("и");
    
                        break;
				    default:
					    russian.append("и");
					    break;
				    }
                }

				break;
			}
			case 'j':
			{
                if( src_1 == end ) {
					russian.append("дж");
                } else if( c_prev =='d' ) {
					russian.append("ж");
                } else {
				    switch (c1)
				    {
                    case 'j': russian.append("дж"); s+=1; break;
                    case 'u': russian.append( c_prev?"джу":"джа"); s+=1; break;
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
                if( s[1] == 'n' && s[2] =='o' && s[3] == 'w' ) {
                    russian.append((s+=3,"ноу"));
                } else {
				switch (c1)
				{
				case 'n':
					russian.append("н");
					++s;
					break;
				default:
					russian.append("к");
					break;
				}
                }

				break;
			}
			case 'l':
			{
                if( s[1] =='i' && s[2] =='v' && s[3] =='e' ) {
					russian.append((s+=2,"лив"));
                } else if(s[1] =='e' && terminating_char(s[2]) ) { // le
					russian.append((++s,"л"));
                } else {
				switch (c1)
				{
				default:
					russian.append("л");
					break;
				}
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
                if( s[1] =='c' && s[2]=='e' && terminating_char(s[3]) ) {
                    russian.append((++s,"нс"));
                } else {
				switch (c1)
				{
				default:
					russian.append("н");
					break;
				}
                }

				break;
			}
			case 'o':
			{
				if ( c_prev == 'r' && c_prev_1 =='o'  && s[1]=='u' && s[2] =='g' && s[3] =='h' ) { // one
                    russian.append( "оу" ) ;
                    s+=3;
				} else if ( s[1]=='n' && s[2] =='e' && (!c_prev || strchr("yeo",c_prev)) ) { // one
                    russian.append( "ван" ) ;
                    s+=2;
				} else if (s[1]=='w') { //
                    const char* x="ау";
                    switch(s[2]) {
                    case 'n': x=(c_prev ? "ау": "оу" ); break;
                    }
                    russian.append( (++s,x) ) ;
				} else if (s + 4 <= end && c1 == 'u' && s[3] == 'e' ) { // ouXe[$]
                    russian.append( "ау" ) ;
                    russian.append( getSingleEnCharTranslit( s[2] ) );
                    s+= 3;
                    break;
				} else if (s + 4 <= end && c1 == 'u' && s[2] == 'g' && s[3] == 'h') { // ough
					if (s + 4 < end) {
						russian.append( s[4]=='t' ? "о":"оу");
					} else
						russian.append("аф");
					s += 3;
					break;
				} else if( s +2 <= end ) {
                    if( c1 == 'o' ) russian.append( (++s, "у") ) ;
                    else if( c1 == 'u' ) russian.append( (++s,"ау") ) ;
                    else russian.append( "о" ) ;
                } else {
					russian.append(c0default);
                }

				break;
			}
			case 'p':
			{
				switch (c1)
				{
				case 'h': russian.append((++s,"ф")); break;
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
					++s;
					break;
				default:
					russian.append("к");
					break;
				}

				break;
			}
			case 'r':
			{
                if( s[1] == 'u' && s[2] == 's' && s[3] =='s' ) {
                    russian.append("ра" );
                    ++s;
                } else if( s[1] == 'e' && s[2] == 'a' && s[3] == 'd' && !terminating_char(c_prev) ) {
                    russian.append( "ред" );
                    s+=3;
                } else if( s[1] == 'o' && s[2] == 'w' && !c_prev ) {
                    russian.append((s+=2,"роу"));
                } else {
			    russian.append("р");
                if( (c1 == 'h'&&(s[2]=='e'||terminating_char(s[2]))) || c1=='r' || (c1 == 'e' && terminating_char(s[2])) ) 
                    ++s;
                }
				break;
			}
			case 's':
			{
				if ( s[1]=='t'&&s[2]=='e'&&s[3]=='a'&&s[4]=='k') { // steak
					russian.append("стейк");
                    s+=4;
				} else if (s[1] =='s' && s[2] =='i' &&  isVowel(s[3]) ) {
                    if( terminating_char(s[4]) ) 
					    russian.append((s+=3,"ша"));
                    else 
					    russian.append((s+=2,"ш"));
				}else if (s[1] =='c' && s[2]=='i') {
					russian.append((s+=2,"сай"));
				}else if (s[1] =='i' && s[2] =='v' && s[3] =='e' ) {
                    s+=3;
					russian.append("сив");
				}else if (s[1] =='o' && s[2] =='m' && s[3] =='e' ) {
                    s+=3;
					russian.append("сам");
				}else if (isVowel(s[1]) && isVowel(c_prev)  ) {
					russian.append("з");
                } else 
				if (s + 2 < end && c1 == 'c' && s[2] == 'h')
				{
					russian.append("щ");
					s += 2;
				} else 
				if (isVowel(prev) && isVowel(c1))
				{
					russian.append("с");
				} else {

				switch (c1)
				{
				case 'h': russian.append("ш"); ++s; break;
				default:
					russian.append("с");
					break;
				}
                }

				break;
			}
			case 't':
			{
                if( s[1] =='i' && s[2] =='o' && s[3] =='n' ) {
                    russian.append("шен");
                    s+=3;
                } else if( s[1] =='h' && s[2] =='r' && s[3] =='o' && s[4] =='w' ) {
					russian.append((s+=4,"троу"));
                } else if( s[1] =='r' && s[2] =='o' && s[3] =='u' && s[4]=='g' && s[5] =='h') {
					russian.append((s+=4,"тру"));
                } else if( s[1] =='i' && s[2] =='v' && s[3] =='e') {
                    russian.append((s+=3,"тив"));
                } else if( s[1] =='i' && s[2] =='a' && s[3] =='l' ) {
                    russian.append("шиал");
                    s+=3;
                } else if( s[1] =='h' && s[2] =='r' ) {
                    russian.append("т");
                    s+=1;
                //// add T* patterns above this line
                } else if( s[1] =='h' && (s[2] =='e'||!c_prev)  ) {
					russian.append((s+=1,"з"));
                } else if (s + 2 < end && c1 == 'c' && s[2] == 'h')
				{
					russian.append("ч");
					s += 2;
				} else {

				switch (c1)
				{
				case 'z':
				case 's':
					russian.append("тс");
					++s;
					break;
				case 'h':
					if (s + 2 < end && isVowel(s[2]))
						russian.append("з");
					else
						russian.append("т");
					++s;
					break;
				default:
					russian.append("т");
					break;
				}
                }

				break;
			}
			case 'u':
			{
                if( !c_prev ) {
                    russian.append( s[1] && !isVowel(s[1]) && s[2] && isVowel(s[2]) ? "ю":"а");
                } else if ( s[1] =='i' ) {
                    ++s;
                    russian.append((c_prev=='q'||c_prev=='s') ? "уи":"и"); 
                } else if ( s[1] =='s' && s[2] =='t' && c_prev == 'j' ) {
                    russian.append("а");
                } else if ( s[1] =='p' && (terminating_char(s[2])||!isVowel(s[2]))  ) {
                    russian.append("а");
                } else if ( s[1] =='d' && s[2] =='e' && s[3] != 'r' ) {
                    russian.append("юд"); 
                    s+=2;
                } else if ( s[1] && !isVowel(s[1]) && c_prev && !isVowel(c_prev) && (s[2] =='e' || s[2] =='i') ) {
					russian.append("ю");
                } else if ( (s[1] =='n' ||s[1] =='m' ||s[1] =='s') && (s[2]=='p'||s[2] =='d'||s[2]=='t') ) {
                    russian.append("а");
                } else if ( s[1] == 'a' ) {
                    russian.append("у");
                } else if ( s[1] && s[1] == s[2] ) {
					russian.append( (c_prev=='r'||c_prev=='p')&&(s[1]=='s'||s[1]=='d') ? "а":"у");
                } else if (c1 && !isVowel(c1) && !isVowel(s[2]) )
				{
                    if( c_prev && strchr( "nbdtprfm", c_prev ) )
					    russian.append( c1 == 's' ? "у": "а");
                    else 
                        russian.append("у");
					break;
				} else {
					russian.append("а");
                }

				break;
			}
			case 'v':
			{
                if( s[1] == 'y' && !isVowel(s[2]) ) {
                    russian.append((++s,"ви"));
                } else {
				switch (c1)
				{
				default:
					russian.append("в");
					break;
				}
                }

				break;
			}
			case 'w':
			{
                if( s[1] =='a' && s[2]=='r' && s[3] =='e' ) {
                    russian.append( "варе" );
                    s+=3;
                } else if( s[1] =='h' && s[2] =='a' && s[3]=='t'   ) { // wh
                    russian.append( (s+=3,"вот"));
                } else if( s[1] =='a' && s[2]=='t' && s[3] =='e' && !c_prev  ) { // wate
                    russian.append( (s+=3,"воте"));
                } else if( s[1] =='r'  ) { // wr
                    russian.append( (++s,"р") );
                } else {
				switch (c1)
				{
                case 'h': 
                    if( s[2] == 'o' ) {
					    russian.append("хо");
                        s+=2;
                    } else {
                        ++s;
					    russian.append("в");
                    }
                    break;
				default:
					russian.append(s[1] ? "в" : "у");
					break;
				}
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
                if( s[1] && !isVowel(s[1]) && isVowel(s[2]) ) {
                    russian.append("ай");
                }  else if (prev && !isVowel(prev)) {
                    const char* x= "и";
                    switch(c1) {
                    case 'a': ++s; x="ья"; break;  
                    case 'e': ++s; x="ье"; break;  
                    case 'u': ++s; x="ью"; break;  
                    case 'o': 
                        if( prev=='r' && s[1] == 'o' && s[2] =='n' && s[3] == 'e' ) {
                            x= "и";
                        } else {
                        ++s; x="ьё"; 
                        }
                        break;  
                    case 'i': ++s; x="ьи"; break;  
                    default: x= "и"; break;  
                    }
                    russian.append(x);
				} else if( !c_prev ) {
                    switch(c1) {
                    case 'a': russian.append( (++s,"я") ); break;
                    case 'o': russian.append( (++s,"йо") ); break;
                    case 'e': russian.append( (++s,"е") ); break;
                    case 'u': russian.append( (++s,"ю") ); break;
                    default: russian.append("й"); break;
                    }
				} else if( !c_prev || isVowel(c_prev) ) {
                    russian.append("й");
                } else {
				    switch (c1)
				    {
				    default:
					    russian.append( "и" );
					    break;
				    }
                }

				break;
			}
			case 'z':
			{
				switch (c1)
				{
				case 'z':
					russian.append("ц");
					++s;
					break;
				case 'h':
					russian.append("ж");
					++s;
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
			++s;
		}
	}
}
}
