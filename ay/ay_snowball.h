/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <stdint.h>
#include <string>
#include <map>
/// data structures and functions implementing multithreaded c++ wrapper over the standard snowball 
#include <vector>
// #include <boost/thread.hpp>
#include <pthread.h>
#include <string.h>

/// stemmer 
struct sb_stemmer;
namespace ay {

/// snowball wrappers 
class StemWrapper {
	sb_stemmer *m_stemmer;
public:
    /// lang is one of the 
    /// 
	enum StemmingLang
	{
		LG_ENGLISH = -1,// "en"
		LG_INVALID,
		LG_FRENCH,      // "fr"
		LG_SPANISH,     // "es"
		LG_GERMAN,
		LG_SWEDISH
	};
    static const char* getValidLangString( int lang ) ;
    static int getLangFromString( const char* langStr );
	static bool isUnicodeLang (int lang);

    static sb_stemmer* mkSnowballStemmer( int lang );
    static void        freeSnowballStemmer( sb_stemmer* sb );

    void setSnowball( sb_stemmer* sb=0 ) { m_stemmer= sb; }
    sb_stemmer*       snowball() {  return m_stemmer; }
    const sb_stemmer* snowball() const {  return m_stemmer; }

    StemWrapper(): m_stemmer(0) {}
    explicit StemWrapper(sb_stemmer* sb): m_stemmer(sb) {}

	inline bool isValid() const { return m_stemmer; }

	bool stem(const char *in, size_t length, std::string& out) const;
};

class MultilangStem {
	typedef std::map<int, StemWrapper> stemmers_t;
	stemmers_t m_stemmers;
public:

	inline const StemWrapper* getStemmer(int lang) const
	{
		stemmers_t::const_iterator pos = m_stemmers.find(lang);
		return pos != m_stemmers.end () ? &pos->second : 0;
	}
	void addLang( int langId );
	bool stem(int lang, const char *in, size_t length, std::string& out, bool fallback) const;

    void clear();
};

class StemThreadPool {
	typedef std::map<pthread_t, MultilangStem> multilangs_t;
	multilangs_t m_multilangs;
	typedef std::vector< int > LangVec;
	LangVec langs;
	
	StemThreadPool()
	{
	}
	
	StemThreadPool(const StemThreadPool&);
	StemThreadPool& operator=(const StemThreadPool&);
public:
	static StemThreadPool& inst()
	{
		static StemThreadPool pool;
		return pool;
	}

	/** This one should better be called before all the threads are created
	 * and in run state.
	 */
	inline void addLang( int lang )
	{
		for (multilangs_t::iterator i = m_multilangs.begin(), end = m_multilangs.end();
				i != end; ++i)
			 i->second.addLang(lang);
		
		langs.push_back(lang);
	}

	inline void createThreadStemmer(const pthread_t id)
	{
		multilangs_t::iterator pos = m_multilangs.find(id);
		if (pos != m_multilangs.end())
			return;

		MultilangStem stem;
		for ( LangVec::const_iterator i = langs.begin(); i != langs.end(); ++i)
			stem.addLang( *i );
		m_multilangs[id] = stem;
	}

	inline const MultilangStem* getThreadStemmer() const
	{
		const pthread_t thisThr = pthread_self();
		multilangs_t::const_iterator pos = m_multilangs.find(thisThr);
		return pos != m_multilangs.end() ? &pos->second : 0;
	}
};
} // namespace ay 
