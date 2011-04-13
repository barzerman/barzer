#include <barzer_shell.h>
#include <barzer_parse.h>
#include <barzer_dtaindex.h>
#include <ay_util_time.h>
#include <sstream>

#include <barzer_el_xml.h>
#include <barzer_el_trie.h>
#include <barzer_el_parser.h>
#include <barzer_el_wildcard.h>
#include "ay/ay_logger.h"
#include <algorithm>
#include <barzer_universe.h>
namespace barzer {

struct BarzerShellContext : public ay::ShellContext {
	DtaIndex* dtaIdx;

	Barz barz;
	QParser parser;

	StoredUniverse universe;

	DtaIndex* obtainDtaIdx() 
	{ return &(universe.getDtaIdx()); }
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
			fp << 
			dtaIdx->resolveStoredTokenStr(ent->euid.tokId)
			<< '|' << *ent << std::endl;
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
static int bshf_lex( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;
	QParser& parser = context->parser;

	ay::InputLineReader reader( in );
	QuestionParm qparm;
	std::ostream& outFp = shell->getOutStream() ;
	while( reader.nextLine() && reader.str.length() ) {
		const char* q = reader.str.c_str();

		/// tokenize
		parser.tokenize_only( barz, q, qparm );
		const TTWPVec& ttVec = barz.getTtVec();
		outFp << "Tokens:" << ttVec << std::endl;

		/// classify tokens in the barz
		parser.lex_only( barz, qparm );
		const CTWPVec& ctVec = barz.getCtVec();
		outFp << "Classified Tokens:\n" << ctVec << std::endl;
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

// just a test function for trie loading
static int bshf_trieloadxml( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	//BarzerShellContext * context = shell->getBarzerContext();
	//DtaIndex* dtaIdx = context->obtainDtaIdx();

	ay::UniqueCharPool strPool;
	
	BarzelRewriterPool brPool( 64*1024 );
	BarzelWildcardPool wcPool;
	BELTrie trie( &brPool, &wcPool );
	BELReader reader(&trie, &strPool);
	reader.initParser(BELReader::INPUT_FMT_XML); 
	
	std::string tmp;
	ay::stopwatch totalTimer;
	
	while( in >> tmp ) {
		ay::stopwatch tmr;
		const char* fname = tmp.c_str();
		//dtaIdx->loadEntities_XML( fname );
		int numsts = reader.loadFromFile(fname);
		std::cerr << numsts << " statements read. ";
		std::cerr << "done in " << tmr.calcTime() << " seconds\n";

	}
	std::cerr << "All done in " << totalTimer.calcTime() << " seconds\n";
	return 0;
}

static int bshf_setloglevel( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	std::map<std::string,int> m;
	int i = 0;
	m["DEBUG"] = i++;
	m["WARNING"] = i++;
	m["ERROR"] = i++;
	m["CRITICAL"] = i++;

	std::string  lstr;
	in >> lstr;
	AYLOG(DEBUG) << "got " << lstr;
	std::transform(lstr.begin(), lstr.end(), lstr.begin(), ::toupper);
	int il = m[lstr];
	AYLOG(DEBUG) << "got " << il << " out of it";
	AYLOG(DEBUG) << "Setting log level to " << ay::Logger::getLogLvlStr( il ) << std::endl;
	ay::Logger::LEVEL = il;
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
	CmdData( (ay::Shell_PROCF)bshf_lex, "lex", "tokenize and then classify (lex) the input" ),
	CmdData( (ay::Shell_PROCF)bshf_tokenize, "tokenize", "tests tokenizer" ),
	CmdData( (ay::Shell_PROCF)bshf_xmload, "xmload", "loads xml from file" ),
	CmdData( (ay::Shell_PROCF)bshf_tok, "tok", "token lookup by string" ),
	CmdData( (ay::Shell_PROCF)bshf_tokid, "tokid", "token lookup by string" ),
	CmdData( (ay::Shell_PROCF)bshf_euid, "euid", "entity lookup by euid (tok class subclass)" ),
	CmdData( (ay::Shell_PROCF)bshf_entid, "entid", "entity lookup by entity id" ),
	CmdData( (ay::Shell_PROCF)bshf_trieloadxml, "trieloadxml", "loads a trie from an xml file" ),
	CmdData( (ay::Shell_PROCF)bshf_setloglevel, "setloglevel", "set a log level (DEBUG/WARNINg/ERROR/CRITICAL)" ),

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
