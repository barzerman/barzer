/// barzer_shell.cpp extension - the original file is too long 
#include <barzer_shell.h>
#include <ay/ay_cmdproc.h>
namespace barzer {
static int bshf_test01( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    std::cerr << "test01\n";
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
static int bshf_morph( BarzerShell* shell, ay::char_cp cmd, std::istream& in , const std::string& argStr)
{
    ay::CmdLineArgsContainer arg( cmd, in );

    ay::file_ostream outFile(shell->getOutStreamPtr()), 
        errFile( shell->getErrStreamPtr() );
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

static const ay::Shell::CmdData g_cmd[] = {
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_test01), "test01", "placeholder" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_morph), "morph", "morphological normalization" ),
ay::Shell::CmdData( (ay::Shell_PROCF)(bshf_file), "file", "-o out file -e err file -i input file. no arguments prints current names" )   

};
std::pair<const ay::Shell::CmdData*, const ay::Shell::CmdData*>  shell_get_cmd01() { 
    return std::pair<const ay::Shell::CmdData*, const ay::Shell::CmdData*>( ARR_BEGIN(g_cmd),ARR_END(g_cmd) );
}

} // end of namespace barzer
