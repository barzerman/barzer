#include <barzer_shell.h>
#include <barzer_parse.h>
#include <barzer_dtaindex.h>
#include <ay_util_time.h>
#include <sstream>

namespace barzer {

struct BarzerShellContext : public ay::ShellContext {
	DtaIndex* dtaIdx;

	Barz barz;
	QParser parser;

	ay::UniqueCharPool charPool;

	BarzerShellContext() : dtaIdx(0) 
	{ }
	~BarzerShellContext() {
		if( dtaIdx ) 
			delete dtaIdx;
	}
	DtaIndex* obtainDtaIdx() 
	{
		if( !dtaIdx ) 
			dtaIdx = new  DtaIndex(&charPool);
		return dtaIdx;
	}
	void clearDtaIdx() 
	{
		if( dtaIdx )
			delete dtaIdx;
	}
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

static int bshf_tokid( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	const DtaIndex* dtaIdx = context->obtainDtaIdx();

	char  tidBuf[ 128 ];
	std::ostream& fp = shell->getOutStream();
	std::string tmp;
	while( in >>tmp )  {
		std::stringstream sstr( tmp );
		while( sstr.getline(tidBuf,sizeof(tidBuf)-1,',') ) {
			StoredTokenId tid = atoi(tidBuf);
			const StoredTokenPool* tokPool = &(dtaIdx->tokPool);
			if( !tokPool->isTokIdValid(tid) ) {
				std::cerr << tid << " is not a valid token id\n";
				continue;
			}
			dtaIdx->printStoredToken( fp, tid );
			fp << std::endl;
		
			//const StoredToken& tok = tokPool->getTokById( tid );
			//fp << tok << std::endl;
		}
	}
	return 0;
}
static int bshf_tok( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	DtaIndex* dtaIdx = context->obtainDtaIdx();
	std::string tmp;
	ay::stopwatch totalTimer;
	in >> tmp;
	const StoredToken* token = dtaIdx->getTokByString( tmp.c_str() );
	if( !token ) {
		std::cerr << "no such token \"" << tmp << "\"\n";
		return 0;
	}
	std::ostream& fp = shell->getOutStream();
	fp << *token << std::endl;
	
	return 0;
}
static int bshf_entid( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	DtaIndex* dtaIdx = context->obtainDtaIdx();
	std::string tmp;
	char  tidBuf[ 128 ];
	std::ostream& fp = shell->getOutStream();
	while( in >>tmp )  {
		std::stringstream sstr( tmp );
		while( sstr.getline(tidBuf,sizeof(tidBuf)-1,',') ) {
			StoredEntityId tid = atoi(tidBuf);
			const StoredEntityPool* entPool = &(dtaIdx->entPool);
			const StoredEntity* ent = entPool->getEntByIdSafe( tid );
			if( !ent ) {
				std::cerr << tid << " is not a valid entity id\n";
				return 0;
			}
			fp << '[' ;
			dtaIdx->printEuid( fp, ent->euid );
			fp << ']' << *ent << std::endl;
		}
	}

	return 0;
}
static int bshf_euid( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	DtaIndex* dtaIdx = context->obtainDtaIdx();
	StoredEntityUniqId euid;
	if( !dtaIdx->buildEuidFromStream(euid,in) ) {
		std::cerr << "invalid euid\n";
		return 0;
	}
	const StoredEntity* ent = dtaIdx->getEntByEuid( euid );
	if( !ent ) {
		std::cerr << "euid " << euid << " not found\n";
		return 0;
	}
	std::ostream& fp = shell->getOutStream();
	fp << *ent << std::endl;

	return 0;
}

static int bshf_xmload( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	DtaIndex* dtaIdx = context->obtainDtaIdx();

	std::string tmp;
	ay::stopwatch totalTimer;
	while( in >> tmp ) {
		ay::stopwatch tmr;
		const char* fname = tmp.c_str();
		dtaIdx->loadEntities_XML( fname );
		std::cerr << "done in " << tmr.calcTime() << " seconds\n";
	}
	std::cerr << "All done in " << totalTimer.calcTime() << " seconds\n";
	return 0;
}
static int bshf_inspect( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	DtaIndex* dtaIdx = context->obtainDtaIdx();
	
	std::cerr << "per token: StoredToken " << sizeof(StoredToken) << " bytes\n";
	std::cerr << "per entity: StoredEntity " << sizeof(StoredEntity) << " bytes\n";
	std::cerr << "extra per entity token: EntTokenOrderInfo " << sizeof(EntTokenOrderInfo) << "+ TokenEntityLinkInfo() " << sizeof(TokenEntityLinkInfo) << " bytes\n";

	if( dtaIdx ) {
		std::ostream& fp = shell->getOutStream();
		dtaIdx->print(fp);
	}
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
	// CmdData( ay::Shell::cmd_set, "set", "sets parameters" ),
	CmdData( ay::Shell::cmd_exit, "exit", "exit the barzer shell" ),
	CmdData( ay::Shell::cmd_exit, "quit", "exit the barzer shell" ),
	CmdData( ay::Shell::cmd_run, "run", "execute script(s)" ),
	CmdData( bshf_test, "test", "just a test" ),
	CmdData( (ay::Shell_PROCF)bshf_inspect, "inspect", "inspects types as well as the actual content" ),
	CmdData( (ay::Shell_PROCF)bshf_tokenize, "tokenize", "tests tokenizer" ),
	CmdData( (ay::Shell_PROCF)bshf_xmload, "xmload", "loads xml from file" ),
	CmdData( (ay::Shell_PROCF)bshf_tok, "tok", "token lookup by string" ),
	CmdData( (ay::Shell_PROCF)bshf_tokid, "tokid", "token lookup by string" ),
	CmdData( (ay::Shell_PROCF)bshf_euid, "euid", "entity lookup by euid (tok class subclass)" ),
	CmdData( (ay::Shell_PROCF)bshf_entid, "entid", "entity lookup by entity id" ),

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
