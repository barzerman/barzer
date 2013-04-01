#pragma once

#include <barzer_entity.h>
#include <ay/ay_bitflags.h>
namespace barzer {

//// comes with every question and is passed to lexers and semanticizers 
struct QuestionParm {
	int lang;  // a valid LANG ID see barzer_language.h
    bool isAutoc; // when true this is an autocomplete query rather than a standard barzer one
    enum : uint8_t {
        STEMMODE_NORMAL,    // standard cautious stemming mode for unmatched tokens during classification
        STEMMODE_AGGRESSIVE // after conservative correction stems until 3 characters are left or anything is matched 
    };
    uint8_t  stemMode;
    
    enum {
        QPBIT_SPELL_NO_SPLITCORRECT,
        QPBIT_ZURCH_FULLTEXT,
        QPBIT_ZURCH_HTML,
        QPBIT_ZURCH_NO_CHUNKS,

        /// add new bits above this line only
        QPBIT_MAX
    };
    enum : int16_t {
        BENI_DEFAULT, // default combined mode (uses heuristics)
        BENI_BENI_ONLY_DEFAULT, // only beni regardless of the flags
        BENI_BENI_ONLY_HEURISTIC, // only beni if universe and query heuristics are met
        BENI_BENI_ENSURE, // barz and beni beni always used regardless of heuristics
        BENI_NO_BENI // no beni no matter what
    };
    int16_t d_beniMode;
    static int16_t parseBeniFlag(const char* s);
    ay::bitflags<QPBIT_MAX> d_biflags;
    
    bool isSplitCorrectionOff( ) const { return d_biflags.checkBit(QPBIT_SPELL_NO_SPLITCORRECT); }
    void turnSplitCorrectionOff( bool x=true ) { d_biflags.set(QPBIT_SPELL_NO_SPLITCORRECT,x); }
    void setZurchFlags( const char* str );
    struct AutocParm {
        uint32_t trieClass, trieId;
        
        enum {
            TOPICMODE_RULES_ONLY,
            TOPICMODE_TOPICS_ONLY,
            TOPICMODE_RULES_AND_TOPICS
        };
        uint32_t topicMode;

        /// when this is not empty only entities whose class/subclass match anything in the vector, will 
        /// be reported by autocomplete - this vector will be very small 
        std::vector< StoredEntityClass > ecVec;
		
		uint32_t numResults;

        bool hasSpecificTrie() const { return( trieClass != 0xffffffff && trieId != 0xffffffff ); }
        bool needOnlyTopic() const { return ( topicMode == TOPICMODE_TOPICS_ONLY ); }
        bool needOnlyRules() const { return ( topicMode == TOPICMODE_RULES_ONLY ); }
        bool needTopic() const { return ( topicMode == TOPICMODE_TOPICS_ONLY || topicMode == TOPICMODE_RULES_AND_TOPICS ); }
        bool needRules() const { return ( topicMode == TOPICMODE_RULES_ONLY || topicMode == TOPICMODE_RULES_AND_TOPICS ); }

        bool needBoth() const { return ( topicMode == TOPICMODE_RULES_AND_TOPICS ); }

        void clear() { trieClass= 0xffffffff; trieId= 0xffffffff;  topicMode=TOPICMODE_RULES_ONLY; }

        bool entFilter( const StoredEntityUniqId& euid ) const ;
        /// parses a string "c1,s1|..."
        void parseEntClassList( const char* s);
        AutocParm() : trieClass(0xffffffff), trieId(0xffffffff), topicMode(TOPICMODE_RULES_ONLY), numResults(10)  {}
    } autoc;

    void setStemMode_Aggressive() { stemMode= STEMMODE_AGGRESSIVE; }
    bool isStemMode_Aggressive() const { return (stemMode==STEMMODE_AGGRESSIVE); }

    void clear() { autoc.clear(); }
	QuestionParm() : 
        lang(0), isAutoc(false), stemMode(STEMMODE_NORMAL), d_beniMode(BENI_DEFAULT) {}
};
} // namespace barzer 
