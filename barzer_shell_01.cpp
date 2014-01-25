/// barzer_shell.cpp extension - the original file is too long 
#include <barzer_shell.h>
#include <ay/ay_cmdproc.h>
#include <barzer_beni.h>
#include <barzer_el_function.h>
#include <batch/barzer_batch_processor.h>
#include <barzer_shellsrv_shared.h>
namespace barzer {
namespace {
    struct Env {
        BarzerShell* shell;
        ay::CmdLineArgsContainer arg;
        ay::file_ostream outFile,errFile;
        ay::file_istream inFile;
        std::string buf;
        BarzerShellContext * context;

        Env( BarzerShell* sh, ay::char_cp cmd, std::istream& in ) : 
            shell(sh),
            arg( cmd, in ) ,
            outFile(shell->getOutStreamPtr()), errFile( shell->getErrStreamPtr()),
            inFile( shell->getInStreamPtr() ),
            context(shell->getBarzerContext())
        {
            arg.getFileFromArg( outFile, "-o" );
            arg.getFileFromArg( errFile, "-e" );
            arg.getFileFromArg( inFile, "-i" );
        }
        BarzerShellContext& ctxt() { return *context; }
        const BarzerShellContext& ctxt() const { return *context; }
        Barz& barz() { return context->barz; }
        const Barz& barz() const { return context->barz; }
        const StoredUniverse* universe_ptr() const { return &(context->getUniverse()); }
        StoredUniverse* universe_ptr() { return &(context->getUniverse()); }

        StoredUniverse& universe() { return (context->getUniverse()); }
        const StoredUniverse& universe() const { return (context->getUniverse()); }
        bool readLine() { return ( std::getline(inFile.fp(),buf)); }

        template <typename T>
        int loop( T&& cb ) 
        {
            while( readLine() ) {
                if(buf.empty() ) break;
                else 
                    cb( *this );
            }
            return 0;
        }
        std::ostream& outFp() { return outFile.fp(); }
        std::istream& inFp() { return inFile.fp(); }
        std::ostream& errFp() { return errFile.fp(); }

        void syncQuestionParm( QuestionParm& qp ) const {  shell->syncQuestionParm(qp) ; }
    };
} // namespace
static int bshf_test01( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    std::string stem;
    Env env( shell, cmd, in );
    env.loop( 
        [&]( Env& env ) -> void 
        {
            std::string out;
            ay::unicode_normalize_punctuation( out, env.buf.c_str(), env.buf.length() );
            env.outFp() << env.buf << ":" << "\"" << out << "\"" << std::endl;
        } 
    );
    std::cerr << "test01\n";
    return 0;
}

static int bshf_batchparse( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    Barz barz;
    std::string tmp;
    in >> tmp;
    BarzerShellSrvShared srvShared( shell->gp );
    srvShared.batch_parse( barz, tmp, std::string(), true );

    return 0;
}
static int bshf_file( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    ay::CmdLineArgsContainer arg( cmd, in );
    if( arg.cmd.argc < 2 ) 
        shell->printStreamState( shell->getErrStream() );

    bool status = false;
    if( const char* x = arg.cmd.getArgVal( status, "-o" ) ) shell->setOutFile( x );
    else if( status ) shell->setOutFile();
        
    if( const char* x = arg.cmd.getArgVal( status, "-e" ) ) shell->setErrFile( x );
    else if( status ) shell->setErrFile();

    if( const char* x = arg.cmd.getArgVal( status, "-i" ) ) shell->setInFile( x );
    else if( status ) shell->setInFile();
    return 0;
}
static int bshf_benix( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    return Env(shell, cmd, in).loop( [&]( Env& env ) -> void {
        TFE_ngram xtractor;
        ExtractedStringFeatureVec fv;
        xtractor( fv, env.buf.c_str(), env.buf.length(), LANG_UNKNOWN );
        for( const auto& i : fv ) {
            env.outFp() << i << std::endl;
        }
    });
}
static int bshf_normalize( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    return Env(shell, cmd, in).loop( [&]( Env& env ) -> void {
        std::string stem;
        BENI::normalize( stem, env.buf, env.universe_ptr() );
        env.outFp() << stem << std::endl;
    });
}

static int bshf_brzstr( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    Env env( shell, cmd, in );
    QParser parser( env.universe() );

    QuestionParm qparm;
    env.syncQuestionParm(qparm);

    return env.loop( [&]( Env& env ) -> void 
        {
            parser.parse( env.barz(), env.buf.c_str(), qparm );
            env.outFp() << env.barz().chain2string() << std::endl;
            env.barz().clearWithTraceAndTopics();
        } 
    );
}
static int bshf_morph( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    return Env( shell, cmd, in ).loop( [&]( Env& env ) -> void {
        std::string stem;
        if( bool stemSuccess = env.universe().getBZSpell()->stem( stem, env.buf.c_str() ) ) 
            env.outFp() << stem << "|" << env.buf << std::endl;
    });
}

static int bshf_func( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    ay::file_ostream outFile(shell->getOutStreamPtr()), errFile( shell->getErrStreamPtr() );
    BELFunctionStorage::help_list_funcs_json( outFile.fp(), shell->getBarzerContext()->gp );
    return 0;
}

static const ay::Shell::CmdData g_cmd[] = {
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_test01), "test01", "placeholder" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_morph), "morph", "morphological normalization" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_brzstr), "brzstr", "transform string using barz (fluff/entity/erc removal)" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_benix), "benix", "extract beni features" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_normalize), "beninorm", "morphological normalization" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_batchparse), "batchparse", "[parms.xml] parses bulk input. params.xml contains parsing parameters" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_file), "file", "-o out file -e err file -i input file. no arguments prints current names" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_func), "func", "lists all built in barzel functions" )   

};
std::pair<const ay::Shell::CmdData*, const ay::Shell::CmdData*>  shell_get_cmd01() { 
    return std::pair<const ay::Shell::CmdData*, const ay::Shell::CmdData*>( ARR_BEGIN(g_cmd),ARR_END(g_cmd) );
}

} // end of namespace barzer
