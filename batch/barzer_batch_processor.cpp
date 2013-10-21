#include <batch/barzer_batch_processor.h>
#include <barzer_parse.h>
#include <barzer_barz.h>
#include <barzer_shell.h>
#include <ay/ay_util_time.h>

namespace barzer {

std::istream& BatchProcessorSettings_Shell::inFP() 
    { return d_shell.getInStream(); }

std::ostream& BatchProcessorSettings_Shell::outFP() 
    { return d_shell.getOutStream(); }

std::ifstream* BatchProcessorSettings::setInFP( const  char* fname )
{
    cleanInFP();
    std::ifstream* fp = ( fname ? new std::ifstream : 0 );
    if( fp ) {
        d_inFP = fp;
        fp->open( fname );
    } else 
        d_inFP= &std::cin;
    return fp;
}
std::ofstream* BatchProcessorSettings::setOutFP( const char* fname )
{
    cleanOutFP();
    std::ofstream* fp = ( fname ? new std::ofstream : 0 );
    if( fp ) {
        d_outFP = fp;
        fp->open( fname );
    } else 
        d_outFP= &std::cout;
    return fp;
}

const StoredUniverse* BatchProcessorSettings::setUniverseById( uint32_t id )
{
    const StoredUniverse* u = d_gp.getUniverse( id );
    setUniverse( u );
    return u;
}

int BatchProcessorZurchPhrases::run( BatchProcessorSettings& settings )
{
    const StoredUniverse* universe =  settings.universePtr();
    if( !universe ) {
            std::cerr << "no valid universe\n";
            return 0;
    }

    QParser parser( *universe );

    size_t numIterations = 1;
    auto& in   = settings.inFP();
    auto& ostr = settings.outFP();
    ay::InputLineReader reader( in );

    QuestionParm&  qparm = settings.qparm();

    ay::stopwatch totalTimer;
    while( reader.nextLine() && reader.str.length() ) {
        ay::stopwatch localTimer;
        const char* q = reader.str.c_str();
        ostr << "parsing: " << q << "\n";

        parser.parse( d_barz, q, qparm );
        ostr << "parsed. printing\n";
        // bs.print(ostr);
        d_barz.clearWithTraceAndTopics();
        // << ttVec << std::endl;
    }
    std::cerr << "All done in " << totalTimer.calcTime() << " seconds\n";
    return 0;
}

} // barzer
