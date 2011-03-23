#ifndef BARZER_LANGUAGE_H  
#define BARZER_LANGUAGE_H  

#include <barzer_parse_types.h>
#include <ay/ay_headers.h>

/// language specific logic 
namespace barzer {

enum {
	LANG_ENGLISH,
	LANG_RUSSIAN,
	LANG_SPANISH,
	LANG_FRENCH,

	LANG_MAX
};

class QSingleLangLexer {
protected:
	int lang;
public:
	virtual int lex( CTWPVec& , const TTWPVec&, const QuestionParm& ) = 0;

	QSingleLangLexer( int lg ) : lang(lg) {}
	virtual ~QSingleLangLexer( ) {}

	struct Error : public QPError { } err;

	/// the factory method
	static QSingleLangLexer* 	mkLexer( int lg ); 
};

class QLangLexer {
	typedef std::vector< QSingleLangLexer* > LangLexVec;

	LangLexVec llVec;
public:
	QLangLexer() : llVec( LANG_MAX, 0 ) {}
	~QLangLexer();
	QSingleLangLexer* addLang( size_t lang );

	QSingleLangLexer* getLang( size_t lang )
		{ return( lang<llVec.size() ? llVec[lang] : 0 ); }

	// routes lexing to an appropriate language specific lexer and lexes
	int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );
};

}
#endif // BARZER_LANGUAGE_H  
