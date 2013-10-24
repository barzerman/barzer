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

int BatchProcessorZurchPhrases::run( BatchProcessorSettings& settings, const char* tail, bool fromShell )
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
    enum {
        PROCESSING_MODE_KEYWORDS
    };
    /// we can easily route by tail here  
    int processingMode = PROCESSING_MODE_KEYWORDS;
    if( tail && strcmp(tail,"keyword") ) {
        std::cerr << "invalid tail \"" << tail << "\" defaulting to keywords passed\n";
    }
    while( std::getline(in,buf) ) {
        ay::stopwatch localTimer;
        ay::parse_separator(
            [&] ( size_t tokN, const char* s, const char* s_end ) 
                { return ( tokN< COL_MAX ?  (col[ tokN ].assign( s, s_end-s ), false): true ); },
            buf.c_str(),
            buf.c_str()+buf.length()
        );
        const char* q = col[ COL_PHRASE_TXT ].c_str();
        if( fromShell )
            ostr << "parsing: " << q << "\n";

        parser.parse( d_barz, q, qparm );
        if( processingMode == PROCESSING_MODE_KEYWORDS ) {
            if( !universe->checkBit(UBIT_NEED_CONFIDENCE) ) 
                d_barz.computeConfidence( *universe, qparm, 0, Barz::ConfidenceMode( StoredEntityClass()) );

            const BarzConfidenceData& confidenceData = d_barz.confidenceData;
            std::vector< std::string > tmp ;
            confidenceData.fillString( tmp, d_barz.getOrigQuestion(), BarzelBead::CONFIDENCE_HIGH );
            if( tmp.size() ) {
                for( size_t i =0; i< tmp.size(); ++i ) {
                    ostr << 
                        col[ COL_DOCID ] << '|' <<
                        col[ COL_TXTTYPE ] << '|' <<
                        col[ COL_PHRASE_NO ] << '.' << i  << '|' << 
                    tmp[i] << std::endl;
                }
            }
        }

        // bs.print(ostr);
        d_barz.clearWithTraceAndTopics();
        // << ttVec << std::endl;
    }
    if( fromShell )
        std::cerr << "All done in " << totalTimer.calcTime() << " seconds\n";
    return 0;
}

} // barzer
