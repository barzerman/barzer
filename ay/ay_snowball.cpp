#include <ay/ay_snowball.h>
extern "C" {
#include "snowball/libstemmer_c/include/libstemmer.h"
}

namespace ay {


int StemWrapper::getLangFromString( const char* langStr )
{
    switch(tolower(langStr[0])) {
    case 'f': 
        if( tolower(langStr[1]) == 'r' ) 
            return LG_FRENCH;
        break;
    case 'e': 
        if( tolower(langStr[1]) == 's' ) 
            return LG_SPANISH;
        break;
    }
        return LG_INVALID;
}
const char* StemWrapper::getValidLangString( int lang ) 
{
    if( lang == LG_FRENCH ) 
        return "fr";
    else if( lang == LG_SPANISH ) 
        return "es";
    else
        return 0;
}
void StemWrapper::freeSnowballStemmer( sb_stemmer* sb )
{
    if( sb )
	    sb_stemmer_delete( sb );
}
sb_stemmer* StemWrapper::mkSnowballStemmer( int lang ) 
{
    const char* lg = getValidLangString(lang);
    return( lg ?  sb_stemmer_new(lg, 0) : 0 );
}

bool StemWrapper::stem (const char* in, size_t len, std::string& out) const
{
	const sb_symbol *sbs = reinterpret_cast<const sb_symbol*> (in);
	const char *result = reinterpret_cast<const char*> (sb_stemmer_stem(m_stemmer, sbs, len));
	if (!result)
		return false;

	out.assign(result, sb_stemmer_length(m_stemmer));
	return !strcmp(in, result);
}

void MultilangStem::addLang( int lang )
{
	if (m_stemmers.find(lang) != m_stemmers.end())
		return;

    sb_stemmer* sb = StemWrapper::mkSnowballStemmer(lang);
    if( sb ) 
        stemmers_t::iterator i = m_stemmers.insert(std::make_pair(lang, StemWrapper(sb))).first;
}

void MultilangStem::clear()
{
    for(stemmers_t::iterator i = m_stemmers.begin(); i!= m_stemmers.end(); ++i ) {
        StemWrapper::freeSnowballStemmer( i->second.snowball() );
    }
    m_stemmers.clear();
}
bool MultilangStem::stem(int lang, const char *in, size_t length, std::string& out) const
{
	stemmers_t::const_iterator pos = m_stemmers.find(lang);
	if (pos != m_stemmers.end() && pos->second.stem(in, length, out))
		return true;

	for (stemmers_t::const_iterator i = m_stemmers.begin(), end = m_stemmers.end(); i != end; ++i)
	{
		if (i->first == lang)
			continue;

		if (i->second.stem(in, length, out))
			return true;
	}

	return false;
}

} // namespace ay
