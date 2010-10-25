#include <barzer_shell.h>

namespace barzer {

typedef ay::Shell::CmdData CmdData;
typedef ay::wchar_cp wchar_cp;
///// specific shell routines
static int bshf_test( ay::Shell*, wchar_cp cmd, std::wistream& in )
{
	std::wcout << L"command: " << cmd << L";";
	std::wcout << L"parms: (";
	std::wstring tmp;
	while( in >> tmp ) {
		std::wcout << tmp << L"|";
	}
	std::wcout << L")" << std::endl;
	return 0;
}

/// end of specific shell routines
static const CmdData g_cmd[] = {
	CmdData( ay::Shell::cmd_help, L"help", L"get help on a barzer function" ),
	CmdData( ay::Shell::cmd_exit, L"exit", L"exit the barzer shell" ),
	CmdData( bshf_test, L"test", L"just a test" ),
};

int BarzerShell::init()
{
	int rc = 0;
	if( !cmdMap.size() )
		rc = indexCmdDataRange(ay::Shell::CmdDataRange( ARR_BEGIN(g_cmd),ARR_END(g_cmd)));

	return rc;
}

} // barzer namespace ends here
