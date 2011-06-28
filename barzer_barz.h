#ifndef BARZER_BARZ_h
#define BARZER_BARZ_h

#include <barzer_el_chain.h>
#include <barzer_parse_types.h>

namespace barzer {

class QSemanticParser;
class QLexParser;
class QTokenizer;

// collection of punits and the original question
class Barz {
	/// original question with 0-s terminating tokens
	/// all poistional info, pointers and offsets from everything
	/// contained in puVec refers to this string. 
	/// so this string is almost always *longer* than the input question
	std::vector<char> question; 
	/// exact copy of the original question
	std::string questionOrig; 
	
	TTWPVec ttVec; 
	CTWPVec ctVec; 
	BarzelBeadChain beadChain;	

	friend class QSemanticParser;
	friend class QLexParser;
	friend class QTokenizer;

	friend class QParser;
	
	/// called from tokenize, ensures that question has 0-terminated
	/// tokens and that ttVec tokens point into it
	void syncQuestionFromTokens();
public:
	const TTWPVec& getTtVec() const { return  ttVec; }
	const CTWPVec& getCtVec() const { return ctVec; }

	BarzelBeadChain& getBeads() { return beadChain; }
	const BarzelBeadChain& getBeads() const { return beadChain; }

	void clear();

	int tokenize( QTokenizer& , const char* q, const QuestionParm& );
	int classifyTokens( QLexParser& , const QuestionParm& );
	int semanticParse( QSemanticParser&, const QuestionParm& );
	int postSemanticParse( QSemanticParser&, const QuestionParm& );

	/// returns pair. first is the number of units which have been modified semantically
	/// mening - fluff, date, entity, erc, expression etc
	// second - number of beads in the barz 
	std::pair<size_t,size_t> getBeadCount() const
	{
		std::pair< size_t, size_t > rv(0,0);
		for( BeadList::const_iterator i = beadChain.getLstBegin(); i!= beadChain.getLstEnd(); ++i ) {
			++(rv.second);
			if( i->isSemantical() ) ++(rv.first);
		}
		return rv;
	}
};
}
#endif // BARZER_BARZ_h
