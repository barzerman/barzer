/// barzer_shell.cpp extension - the original file is too long 
#include <barzer_shell.h>
#include <ay/ay_cmdproc.h>
#include <barzer_beni.h>
#include <barzer_el_function.h>
#include <batch/barzer_batch_processor.h>
#include <barzer_shellsrv_shared.h>
namespace barzer {
static int bshf_test01( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    std::string buf;
    std::string stem;
    ay::file_istream inFile( shell->getInStreamPtr() );
    while( std::getline(inFile.fp(),buf) ) {
        if( buf.empty() )
            break;
        std::string out;
        ay::unicode_normalize_punctuation( out, buf.c_str(), buf.length() );
        shell->getOutStream() << buf << ":" << "\"" << out << "\"" << std::endl;
        
    }
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
    ay::file_istream inFile( shell->getInStreamPtr() );
    ay::file_ostream outFile(shell->getOutStreamPtr()), errFile( shell->getErrStreamPtr() );

    std::string buf;
    while( std::getline(inFile.fp(),buf) ) {
        TFE_ngram xtractor;
        ExtractedStringFeatureVec fv;
        xtractor( fv, buf.c_str(), buf.length(), LANG_UNKNOWN );
        for( const auto& i : fv ) {
            outFile.fp() << i << std::endl;
        }
    }
    return 0;
}
static int bshf_normalize( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    ay::file_istream inFile( shell->getInStreamPtr() );
    ay::file_ostream outFile(shell->getOutStreamPtr()), errFile( shell->getErrStreamPtr() );
    std::string buf;
    std::string stem;
    const auto context = shell->getBarzerContext();

    const StoredUniverse &uni = context->getUniverse();
    while( std::getline(inFile.fp(),buf) ) {
        if( buf.empty() ) 
            break;
        BENI::normalize( stem, buf, &uni );
        outFile.fp() << stem << std::endl;
    }
    return 0;
}
static int bshf_morph( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    ay::CmdLineArgsContainer arg( cmd, in );

    ay::file_ostream outFile(shell->getOutStreamPtr()), errFile( shell->getErrStreamPtr() );
    ay::file_istream inFile( shell->getInStreamPtr() );

    arg.getFileFromArg( outFile, "-o" );
    arg.getFileFromArg( errFile, "-e" );
    arg.getFileFromArg( inFile, "-i" );

	BarzerShellContext *context = shell->getBarzerContext();
	const StoredUniverse &uni = context->getUniverse();
	const BZSpell* bzSpell = uni.getBZSpell();

    bool stemOnly = arg.cmd.hasArg( "-c");
    bool noBreakOnBlank = arg.cmd.hasArg( "-b");

    std::string buf;
    std::string stem;
    while( std::getline(inFile.fp(),buf) ) {
        stem.clear();
        if( buf.empty() && !(noBreakOnBlank ) )
            break;
        if( bool stemSuccess = bzSpell->stem( stem, buf.c_str() ) ) 
            outFile.fp() << stem << "|" << buf << std::endl;
    }

    return 0;
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
