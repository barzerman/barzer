#pragma once
#include <iostream>
#include <fstream>
#include <barzer_question_parm.h>

namespace barzer {
class GlobalPools;
class StoredUniverse;
class Barz;
struct BarzerShell;
struct QuestionParm;

class BatchProcessorSettings_Base {
protected:
    const StoredUniverse* d_universe;
    QuestionParm          d_qparm;
public:
    BatchProcessorSettings_Base(): d_universe(0) {}
    virtual ~BatchProcessorSettings_Base() {}

    int init( const char* fileName ) { return 0; }

    void setUniverse( const StoredUniverse* u )
        { d_universe = u; }

    virtual std::istream& inFP() = 0;
    virtual std::ostream& outFP() = 0;
    QuestionParm& qparm() { return d_qparm; }
    const QuestionParm& qparm() const { return d_qparm; }

    const StoredUniverse* universePtr() { return d_universe; }
};
class BatchProcessorSettings : public BatchProcessorSettings_Base {
    const GlobalPools& d_gp;
    std::istream* d_inFP;
    std::ostream* d_outFP;
    void cleanInFP() { if(d_inFP  != (&std::cin) ) delete d_inFP; d_inFP=0;}
    void cleanOutFP() { if(d_outFP != (&std::cout)) delete d_outFP; d_outFP=0; }

public: 
    BatchProcessorSettings( const GlobalPools& gp )  :
        d_gp( gp ), 
        d_inFP(&std::cin), d_outFP(&std::cout) {}

    ~BatchProcessorSettings() 
    {
        cleanInFP();
        cleanOutFP();
    }
    std::ostream& outFP() { return *d_outFP; }
    std::istream& inFP() { return *d_inFP; }

    std::ifstream* setInFP( const  char* fname ) ;
    std::ofstream* setOutFP( const char* fname );
    const StoredUniverse* setUniverseById( uint32_t id );
};

class BatchProcessorSettings_Shell : public BatchProcessorSettings_Base {
    BarzerShell& d_shell; 
    
public:
    BatchProcessorSettings_Shell( BarzerShell& shell ) : d_shell(shell) {}
    ~BatchProcessorSettings_Shell() {}

    std::istream& inFP();
    std::ostream& outFP();
};

class BatchProcessor {
protected:
    std::string d_curBuffer;
    Barz& d_barz;
public:
    BatchProcessor( Barz& b ) : d_barz(b) {}
    virtual ~BatchProcessor() {}

    const Barz& barz() const { return d_barz; }
          Barz& barz()       { return d_barz; }

    virtual int run( BatchProcessorSettings& ) = 0;
};

class BatchProcessorZurchPhrases : public BatchProcessor {
public:
    ~BatchProcessorZurchPhrases() {}
    BatchProcessorZurchPhrases( Barz& b ) : BatchProcessor(b) {}
    int run( BatchProcessorSettings& );
};


} // namespace barzer 
