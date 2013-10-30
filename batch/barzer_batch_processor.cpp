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

void BatchProcessorZurchPhrases::computeSubtract( const BatchProcessorSettings& settings, std::vector< std::string>& text) const
{
    std::vector<std::pair<size_t, size_t>> chunk;
    const auto universe = settings.universePtr();

    std::vector< const BarzelBead* > beadsToCut;
    const char* meaningfulPunct = "-.!,";
    for( const auto& bead : d_barz.getBeads().lst ) {
        bool cutOut = false;
        bool flushCut = false;
        size_t numTok = bead.getFullNumTokens();
        bool hasSpellCorrections = bead.hasSpellCorrections();
        if( numTok < 2 && hasSpellCorrections  ) {
            flushCut = true;
        } else if( const BarzerEntity* ent = bead.isEntity() ){
            cutOut= true;
            flushCut = true;
        } else if( const BarzerLiteral* l = bead.getLiteral() ) {
            if( l->isStop() && !l->isPunct() )
                beadsToCut.push_back( &bead );
            else if( l->isPunct() || !strchr( meaningfulPunct,l->getPunct()) ) 
                beadsToCut.push_back( &bead );
            else if( !l->isBlank() )
                flushCut = true;
        } else if( const auto* x = bead.get<BarzerNumber>() )  {
            beadsToCut.push_back( &bead );
        } else if( const BarzerString* l = bead.getString() ) {
                const std::string& theS = l->getStr();
            if( l->isFluff() && ( !beadsToCut.empty() || theS.length() > 1 || (theS.length()==1 && !strchr(meaningfulPunct ,theS[0]) ) ) )
                beadsToCut.push_back( &bead );
            else if( !l->getStr().empty() )
                flushCut = true;
        } else if( !bead.isBlank() ) {
            flushCut = true;
        }

        /// flushing 
        if( flushCut && !beadsToCut.empty() ) {
            for( const auto& i : beadsToCut )
                    d_barz.getContinuousOrigOffsets( *i, chunk );
            beadsToCut.clear();
        }
        if( cutOut )
            d_barz.getContinuousOrigOffsets( bead, chunk );
    }
    if( !beadsToCut.empty() ) {
        for( const auto& i : beadsToCut ) 
            d_barz.getContinuousOrigOffsets( *i, chunk );
    }
    d_barz.getAllButSrctok( text, chunk );
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

        parser.parse( d_barz, q, qparm );
        if( processingMode == PROCESSING_MODE_KEYWORDS ) {
            std::vector<std::string> text;
            computeSubtract( settings, text );

            if( text.size() ) {
                size_t i = 0;
                for( const auto& t : text ) {
                    ostr << 
                        col[ COL_DOCID ] << '|' <<
                        col[ COL_TXTTYPE ] << '|' <<
                        col[ COL_PHRASE_NO ] << '.' << i++  << '|' << 
                    t << std::endl;
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
