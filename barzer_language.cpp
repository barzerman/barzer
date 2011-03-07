#include <barzer_language.h>

#include <lg_ru/barzer_ru_lex.h>
#include <lg_en/barzer_en_lex.h>

namespace barzer {

/// the factory method
QSingleLangLexer* 	QSingleLangLexer::mkLexer( int lg )
{
	switch( lg ) {
	case LANG_ENGLISH:
		return new QSingleLangLexer_EN();
	case LANG_RUSSIAN:
		return new QSingleLangLexer_RU();
	}
	return 0;
}

QLangLexer::~QLangLexer()
{
	for( LangLexVec::iterator i = llVec.begin(); i!= llVec.end(); ++i ) { 
		if( *i )  {
			delete *i;
			*i = 0;
		}
	}
}

QSingleLangLexer* QLangLexer::addLang( size_t lang )
{
	if( lang < llVec.size() ) {
		if( !llVec[ lang ] )
			llVec[ lang ] = QSingleLangLexer::mkLexer( lang );
		return llVec[ lang ];
	}
	return 0;
}

int QLangLexer::lex( CTWPVec& cv, const TTWPVec& tv, const QuestionParm& qparm )
{
	QSingleLangLexer* lexer = getLang( qparm.lang );
	if( !lexer ) {
		std::cerr << "QLangLexer cant find lexer for lang " << qparm.lang << std::endl;
		return 666;
	}
	return lexer->lex( cv, tv, qparm );
}

}
