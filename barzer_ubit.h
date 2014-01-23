#pragma once
/// universe specific bits 
namespace barzer {
    enum {
        UBIT_NOSTRIP_DIACTITICS, // when set diacritics wont be stripped and utf8 is going to be processed as is 
        UBIT_TWOSTAGE_TOKENIZER, // when 1 tries to tokenize in 2 stages - first just by spaces then by everything
        UBIT_NO_ENTRELEVANCE_SORT, // when 1 wont sort entities by relevance on output
        UBIT_CORRECT_FROM_DICTIONARY, // when 1 will correct words away from generic dictionary otherwise leaves those words intact
		UBIT_LEX_STEMPUNCT, // when 1, space_default token classifier will consider <term>'s (and other similar) as <term> if <term> is valid
		UBIT_FEATURED_SPELLCORRECT, // when 1, feature extraction-based spell corrector will be active
		UBIT_NEED_CONFIDENCE, // when 1 output will contain confidence info(which needs to be separately computed) user:flags[value] has 'c'
		UBIT_FEATURED_SPELLCORRECT_ONLY, // when 1, old not-feature-extraction-based spell corrector will be disabled

        /// extra normalization includes single quotes, utf8 punctuation etc.
        UBIT_NO_EXTRA_NORMALIZATION, /// when set no extra normalization is performed
        UBIT_USE_BENI_VANILLA, /// BENI search will be used if final bead chain    
        UBIT_USE_BENI_IDS, /// when using beni will try to only invoke for ids 
        UBIT_BENI_SOUNDSLIKE, /// if on BENI stores converts all ngrams using sounds like 
        UBIT_BENI_NO_BOOST_MATCH_LEN, /// when set there is no boost by match length
        UBIT_BENI_NORM_KEEPANDS, /// doesnt remove "ands" 
        UBIT_BENI_NO_COVDROPCUT, /// when on coverage drop is not used as cutoff. by default coverage drop of .15 from the top element is used to cut off
        UBIT_BENI_TOPIC_FILTER, /// when set beni searches will be filtered by topics (if any have been discovered)

        /// number lexing
        UBIT_NC_LEADING_ZERO_ISNUMBER, /// if bit set will interpret 00500 as 500, otherwise only 0X (2 digits are perceived as numbers)
                                       /// controlled by user:flags[value] has 'z'
        UBIT_NC_NO_SEPARATORS, /// all separators and floating point numbers are ignored. 1.1 is parsed as 1 / . / 1 (2 ints nd a dot)
                               /// controlled by user:flags[value] has 's'
        UBIT_NO_SPLIT_DOTDATE, // 

        /// date processing 
        UBIT_DATE_FUTURE_BIAS,
        /// add new bits above this line only 
        UBIT_MAX
    };
} //namespace barzer
