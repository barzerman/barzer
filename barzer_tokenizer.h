#ifndef BARZER_TOKENIZER_H
#define BARZER_TOKENIZER_H

#include <barzer_parse_types.h>
namespace barzer {

class StoredUniverse;
/// TokenizerRuleset is one tokenization algorithm applied to a particular buffer
/// TokenizerStrategy is a combination of strategy information (startegy type etc.) and an array of TokenizerRulesets
/// under one such strategy (called TOKENIZER_STACK) tokenizer rulesets are applied sequentially weak to strong as long as 
/// either unlassified tokens are present or rulesets stack has been exhausted. 
struct TokenizerRuleset {
    enum {
        TRT_BARZER_DEFAULT,/// standard tokenizer 
        TRT_SPACES, /// only tokenize on spaces 
        TRT_CUSTOM, /// custom ruleset with custom rules
        TRT_MAX
    };
    //// we can add more stuff to TokenizerRuleset later
};

struct TokenizerStrategy {
    enum {
        STRAT_TYPE_DEFAULT,
        STRAT_TYPE_SPACE_DEFAULT,
        STRAT_TYPE_CASCADE
    };
    int type; // one of STRAT_TYPE_ constants
    std::vector< TokenizerRuleset > d_ruleset;
    
    bool isDefault() const { return (type==STRAT_TYPE_DEFAULT); }
    int getType() const { return type; }
    TokenizerStrategy() : type(STRAT_TYPE_DEFAULT) {}
    void setType( int i ) { type = i; }
};

/// converts input const char* quesion
/// into vector of TToken with position (token's number in the input)
/// blanks will be there as blank tokens
/// tokenizes on any and all punctuation
class QTokenizer {
    const StoredUniverse& d_universe;
public:
    QTokenizer( const StoredUniverse& u ) : d_universe(u) {}
	enum { MAX_QUERY_LEN = 1024, MAX_NUM_TOKENS = 96 };
	struct Error : public QPError { } err;

	int tokenize( TTWPVec& , const char*, const QuestionParm& );
	int tokenize( Barz& barz, const TokenizerStrategy& , const QuestionParm& );
    //// special (hardcoded) strategy tokenizer
	int tokenize_strat_space( Barz& , const QuestionParm& );
};

} // namespace barzer
#endif //  BARZER_TOKENIZER_H
