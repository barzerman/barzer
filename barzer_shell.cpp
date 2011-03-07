#include <barzer_shell.h>
#include <barzer_parse.h>

namespace barzer {

struct BarzerShellContext : public ay::ShellContext {
	Barz barz;
	QParser parser;

	~BarzerShellContext() {}
};

inline BarzerShellContext* BarzerShell::getBarzerContext()
{
	return dynamic_cast<BarzerShellContext*>(context);  
}


typedef const char* char_cp;
typedef ay::Shell::CmdData CmdData;
///// specific shell routines
static int bshf_test( ay::Shell*, char_cp cmd, std::istream& in )
{
	std::cout << "command: " << cmd << ";";
	std::cout << "parms: (";
	std::string tmp;
	while( in >> tmp ) {
		std::cout << tmp << "|";
	}
	std::cout << ")" << std::endl;
	
	return 0;
}

static int bshf_tokenize( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();

	Barz& barz = context->barz;
	QParser& parser = context->parser;

	ay::InputLineReader reader( in );
	QuestionParm qparm;
	while( reader.nextLine() && reader.str.length() ) {
		barz.tokenize( parser.tokenizer, reader.str.c_str(), qparm );
		const TTWPVec& ttVec = barz.getTtVec();
		shell->getOutStream() << ttVec << std::endl;
	}
	return 0;
}

/// end of specific shell routines
static const CmdData g_cmd[] = {
	CmdData( ay::Shell::cmd_help, "help", "get help on a barzer function" ),
	CmdData( ay::Shell::cmd_exit, "exit", "exit the barzer shell" ),
	CmdData( bshf_test, "test", "just a test" ),
	CmdData( (ay::Shell_PROCF)bshf_tokenize, "tokenize", "tests tokenizer" ),
};

ay::ShellContext* BarzerShell::mkContext()
{
	return new BarzerShellContext();
}

int BarzerShell::init()
{
	int rc = 0;
	if( !cmdMap.size() )
		rc = indexCmdDataRange(ay::Shell::CmdDataRange( ARR_BEGIN(g_cmd),ARR_END(g_cmd)));

	if( context ) 
		delete context;

	context = mkContext();
	return rc;
}

} // barzer namespace ends here
