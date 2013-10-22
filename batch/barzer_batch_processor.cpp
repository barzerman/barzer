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

    QuestionParm&  qparm = settings.qparm();

    ay::stopwatch totalTimer;
    std::string buf;
    /// parsing phrase format DOCID|TXTTYPE|Phrase #|Phrase Text
    /// we will output 0 for Phrase # , preserve DOCID and TXT Type 
    /// but the phrase will be broken into a bunch of smaller subphrases (nohi)
    enum {
        COL_DOCID, 
        COL_TXTTYPE,
        COL_PHRASE_NO,
        COL_PHRASE_TXT,

        COL_MAX
    };
    std::string col[ COL_MAX ];
    while( std::getline(in,buf) ) {
        ay::stopwatch localTimer;
        ay::parse_separator(
            [&] ( size_t tokN, const char* s, const char* s_end ) 
                { return ( tokN< COL_MAX ?  (col[ tokN ].assign( s, s_end-s ), false): true ); },
            buf.c_str(),
            buf.c_str()+buf.length()
        );
        const char* q = col[ COL_PHRASE_TXT ].c_str();
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
