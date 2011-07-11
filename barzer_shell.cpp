#include <barzer_shell.h>
#include <barzer_server.h>
#include <ay_util_time.h>

#include <barzer_el_xml.h>
#include <barzer_el_trie.h>
#include <barzer_el_parser.h>
#include <barzer_el_wildcard.h>
#include <barzer_el_analysis.h>
#include <ay/ay_logger.h>
#include <algorithm>
#include <barzer_el_trie_shell.h>
#include <barzer_server_response.h>
#include <functional>
#include <boost/function.hpp>

#include <barzer_server_request.h>
//
#include <sstream>
#include <fstream>



namespace barzer {


BarzerShellContext* BarzerShell::getBarzerContext()
{
	return dynamic_cast<BarzerShellContext*>(context);  
}


typedef const char* char_cp;
typedef ay::Shell::CmdData CmdData;
///// specific shell routines


static int bshf_test( ay::Shell*, char_cp cmd, std::istream& in )
{
	/// NEVER REMOVE THIS FUNCTION . it's used in debug scripts in those cases 
	/// gdb ctrl-C gets passed to the app - it is unfortunately the case on Mac
	/// - AY
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
static int bshf_emit( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	const StoredUniverse &uni = context->universe;
	const GlobalPools &globalPools = uni.getGlobalPools();

	std::stringstream sstr;

	std::string tmp;
	ay::InputLineReader reader( in );

	std::stringstream strStr;
	while( reader.nextLine() && reader.str.length() ) {
		strStr << reader.str;
	}
	std::string q = strStr.str();
	RequestEnvironment reqEnv(shell->getOutStream(),q.c_str(),q.length());

	request::emit( reqEnv, globalPools );
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
	std::cerr << "Barzel:" << std::endl;
	AYDEBUG( sizeof(BarzelTranslation) );
	AYDEBUG( sizeof(BarzelTrieNode) );
	AYDEBUG( sizeof(BarzelFCMap) );
	std::cerr << "Barzel Wildcards:\n";
	context->getTrie().getWildcardPool().print( std::cerr );

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

static int bshf_spell( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	const StoredUniverse &uni = context->universe;
	const BarzerHunspell& hunspell = uni.getHunspell();
	ay::InputLineReader reader( in );
	BarzerHunspellInvoke spellChecker(hunspell,uni.getGlobalPools());

	while( reader.nextLine() && reader.str.length() ) {
		const char*  str = reader.str.c_str();
		std::pair< int, size_t> scResult = spellChecker.checkSpell( str );	
		if( !scResult.first ) {

			std::cerr << "misspelling correctin it. got " << scResult.second << " suggestions\n";
			const char*const* sugg = spellChecker.getAllSuggestions();
			for( size_t i =0; i< scResult.second; ++i ) {
				std::cerr << sugg[i] << std::endl;
			}
		} else {
			std::cerr << "valid spelling\n";
		}
	}

	return 0;
}

static int bshf_process( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;
	// Barz barz;
	QParser& parser = context->parser;

	BarzStreamerXML bs(barz, context->universe);
	std::string fname;

	std::ostream *ostr = &(shell->getOutStream());
	std::ofstream ofile;

	ay::InputLineReader reader( in );
	if (in >> fname) {
		ofile.open(fname.c_str());
		ostr = &ofile;
	}

	QuestionParm qparm;
	//std::ostream &os = shell->getOutStream();
	const StoredUniverse &uni = context->universe;
	const GlobalPools &globalPools = uni.getGlobalPools();

	while( reader.nextLine() && reader.str.length() ) {
		const char* q = reader.str.c_str();
		*ostr << "parsing: " << q << "\n";
		parser.parse( barz, q, qparm );
		*ostr << "parsed. printing\n";
		bs.print(*ostr);
		
		if( barz.barzelTraceVec.size() ) {
			*ostr << "barzel rules trace {\n";	
			for( Barz::BarzelTraceVec::const_iterator ti = barz.barzelTraceVec.begin(); ti != barz.barzelTraceVec.end(); ++ti ) {
				globalPools.printTanslationTraceInfo( *ostr, *ti ) << "\n";
			}
			*ostr << "end of rule trace }\n";	
		}
		// << ttVec << std::endl;
	}
	return 0;
}
static int bshf_anlqry( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;
	// Barz barz;
	QParser& parser = context->parser;

	BarzStreamerXML bs(barz, context->universe);
	std::string fname;

	std::ostream *ostr = &(shell->getOutStream());
	std::ofstream ofile;

	ay::InputLineReader reader( in );
	if (in >> fname) {
		ofile.open(fname.c_str());
		ostr = &ofile;
	}

	QuestionParm qparm;
	//std::ostream &os = shell->getOutStream();

	size_t numSemanticalBarzes = 0, numQueries = 0;
	ay::stopwatch totalTimer;
	while( reader.nextLine() && reader.str.length() ) {
		const char* q = reader.str.c_str();
		parser.parse( barz, q, qparm );

		std::pair< size_t, size_t > barzRc = barz.getBeadCount();
		++numQueries;
		if( !(numQueries%5000) ) 
			std::cerr << '.';
		if( barzRc.first ) 
			++numSemanticalBarzes;
	}
	std::cout << "\n" << numQueries << " (" << numSemanticalBarzes << " semantical) queries processed read. in "  << totalTimer.calcTime() << std::endl;
	return 0;
}



static int bshf_setloglevel( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	static std::map<std::string,int> m;

	// had to hardcode it after removal of the Logger array ...
	static std::string slevels[]
	                     = {"DEBUG", "WARNING","ERROR","CRITICAL"};

	for (int i = 0; i < ay::Logger::LOG_LEVEL_MAX; i++)
		m[slevels[i]] = i;

	std::string  lstr;
	in >> lstr;
	std::transform(lstr.begin(), lstr.end(), lstr.begin(), ::toupper);
	int il = m[lstr];
	AYLOG(DEBUG) << "got " << il << " out of it";
	AYLOG(DEBUG) << "Setting log level to " << ay::Logger::getLogLvlStr( il ) << std::endl;
	ay::Logger::LEVEL = il;
	return 0;
}


static int bshf_trie( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	return bshtrie_process( shell, in );
}

namespace {

struct ShellState {
	BarzerShellContext *context;
	StoredUniverse &uni ;
	BELTrieWalker &walker ;
	BELTrie &trie ;
    const BarzelTrieNode &curTrieNode;
	ay::UniqueCharPool &stringPool;


	ShellState( BarzerShell* shell, char_cp cmd, std::istream& in ) :
		context(shell->getBarzerContext()),
		uni(context->universe),
		walker(context->trieWalker),
		trie(context->getTrie()),
		curTrieNode(walker.getCurrentNode()),
		stringPool( uni.getStringPool())
	{}

};

} // anon namespace ends 
static int bshf_triestats( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	ShellState sh( shell, cmd, in );
	{
	BarzelTrieTraverser_depth trav( sh.trie.getRoot(), shell->getBarzerContext()->getTrie() );
	BarzelTrieStatsCounter counter;
	trav.traverse( counter, sh.curTrieNode );
	std::cerr << counter << std::endl;
	}
	return 0;
}

namespace {

struct TrieLeafFinder {
	bool operator()( const BarzelTrieNode& t ) const
	{
		return !t.isLeaf();
	}
};

}

static int bshf_trans( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	ShellState sh( shell, cmd, in );
	BELPrintFormat fmt;
	BELPrintContext prnContext( sh.trie, sh.stringPool, fmt );
	
	const BarzelTrieNode* aLeaf = 0;
	/// 
	if (sh.curTrieNode.isLeaf()) {
		aLeaf = &sh.curTrieNode;
	} else {
		BarzelTrieTraverser_depth trav( sh.trie.getRoot(), sh.trie );
		TrieLeafFinder fun;
		aLeaf = trav.traverse( fun, sh.curTrieNode );
	}
	AYDEBUG(sizeof(BTND_RewriteData));
	if( !aLeaf ) {
		AYLOG(ERROR) << "this trie node has no descendants with a valid translation\n";
		return 0;
	} else {
		aLeaf->print( std::cerr, prnContext ) << std::endl;

	}
	return 0;
}

namespace {

struct TAParms {
static size_t nameThreshold; 
static size_t fluffThreshold; 
static size_t maxNameLen; 
};
size_t TAParms::nameThreshold = 2000;
size_t TAParms::fluffThreshold = 200;
size_t TAParms::maxNameLen     = 3;


}

/// data analysis entry point
static int bshf_dtaan( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	std::ostream *ostr = &(shell->getOutStream());
	std::ofstream ofile;

		
	//ay::InputLineReader reader( in );
	std::string str;
	if (in >> str) {
		std::string str1;
		if( str == "nameth" ) {
			if( in >> str1 ) { 
				TAParms::nameThreshold = atoi( str1.c_str() ); 
				std::cerr << "name threshold set to 1/" << TAParms::nameThreshold << "-th\n";
				return 0;
			}
		} else if( str == "maxname" ) {
			if( in >> str1 ) { 
				TAParms::maxNameLen = atoi( str1.c_str() ); 
				std::cerr << "maxname length set to:" << TAParms::maxNameLen << "\n";
				return 0;
			}
		} else if( str == "fluffth" ) {
			if( in >> str1 ) { 
				TAParms::fluffThreshold = atoi( str1.c_str() ); 
				std::cerr << "fluff threshold set to 1/" << TAParms::fluffThreshold << "-th\n";
				return 0;
			}
		} else {
			ofile.open(str.c_str());
			if( !ofile.is_open() ) {
				std::cerr << "failed to open " << str << " for output \n";
			} else {
				std::cerr << "results will be written to " << str << "\n";
				ostr = &ofile;
			}
		}
	}

	ShellState sh( shell, cmd, in );
	UniverseTrieClusterIterator trieClusterIter( sh.uni.getTrieCluster() );
	TrieAnalyzer analyzer( sh.uni, trieClusterIter );
	analyzer.setNameThreshold( TAParms::nameThreshold );
	analyzer.setFluffThreshold( TAParms::fluffThreshold );

	analyzer.traverse(trieClusterIter);

	TANameProducer nameProducer( *ostr );
	nameProducer.setMaxNameLen( TAParms::maxNameLen );
	TrieAnalyzerTraverser< TANameProducer > trav( analyzer,nameProducer);

	std::cerr << "computing ...\n";
	trav.traverse();
	nameProducer.setMode_output();
	trav.traverse();
	
	std::cerr << nameProducer.d_numNames << " names and " << nameProducer.d_numFluff << " fluff patterns saved\n";
	return 0;
}

static int bshf_trls( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->universe;
	BELTrieWalker &walker = context->trieWalker;
	BELTrie &trie = context->getTrie();

	std::vector<BarzelTrieFirmChildKey> &fcvec = walker.getFCvec();
    std::vector<BarzelWCLookupKey> &wcvec = walker.getWCvec();

    const BarzelTrieNode &node = walker.getCurrentNode();

	ay::UniqueCharPool &stringPool = uni.getStringPool();
	BELPrintFormat fmt;
	BELPrintContext prnContext( trie, stringPool, fmt );

	TrieNodeStack &stack = walker.getNodeStack();
	std::cout << "/";
	if (stack.size() > 1) {
		for (TrieNodeStack::iterator si = ++(stack.begin()); si != stack.end(); ++si) {
			BELTrieWalkerKey &key = (*si).first;
			switch(key.which()) {
			case 0: //BarzelTrieFirmChildKey
				boost::get<BarzelTrieFirmChildKey>(key).print(std::cout, prnContext);
				break;
			case 1: //BarzelWCLookupKey
				BarzelWCLookupKey &wckey = boost::get<BarzelWCLookupKey>(key);
				prnContext.printBarzelWCLookupKey(std::cout, wckey);
				break;
			}
			std::cout << "/";
		}
	}
	std::cout << std::endl;

	if( fcvec.size() ) {
		std::cout << fcvec.size() << " firm children" << std::endl;

		for (size_t i = 0; i < fcvec.size(); i++) {
			BarzelTrieFirmChildKey &key = fcvec[i];
			std::cout << "[" << i << "] ";
			key.print(std::cout, prnContext) << std::endl;
		}
	} else {
		std::cout << "<no firm>" << std::endl;
	}

	if( wcvec.size() ) {
		std::cout << wcvec.size() << " wildcards lookup[" << 
		std::hex << node.getWCLookupId() << "]" << std::endl;
	} else {
		std::cout << "<no wildcards>" << std::endl;
	}

	for (size_t i = 0; i < wcvec.size(); i++) {
		//const BarzelWCLookupKey &key = wcvec[i];
		std::cout << "[" << i << "] ";
		prnContext.printBarzelWCLookupKey(std::cout, wcvec[i]);
		std::cout << std::endl;

	}

	if (node.isLeaf()) {
		std::cout << "*LEAF*:";
		node.print_translation(std::cout, prnContext) << std::endl;
	}
	//AYLOG(DEBUG) << "Stack size is " << walker.getNodeStack().size();

	return 0;
}

static int bshf_trcd( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BELTrieWalker &w = shell->getBarzerContext()->trieWalker;
	std::string s;
	if (in >> s) {
		const char *cstr = s.c_str();
		size_t num = atoi(cstr);
		//std::cout << "Moving to child " << num << std::endl;
		if (w.moveToFC(num)) {
			AYLOG(ERROR) << "Couldn't load child";
		}
		//std::cout << "Stack size is " << w.getNodeStack().size() << std::endl;
	} else {
		std::cout << "trie moveto <#Child>" << std::endl;
	}
	return bshf_trls(shell, cmd, in );
}

static int bshf_trcdw( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BELTrieWalker &w = shell->getBarzerContext()->trieWalker;
	std::string s;
	if (in >> s) {
		const char *cstr = s.c_str();
		size_t num = atoi(cstr);
		//std::cout << "Moving to wildcard child " << num << std::endl;
		if (w.moveToWC(num)) {
			AYLOG(ERROR) << "Couldn't load the child";
		}
	} else {
		std::cout << "trie moveto <#Child>" << std::endl;
	}
	return bshf_trls(shell, cmd, in );;
}


static int bshf_trup( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BELTrieWalker &w = shell->getBarzerContext()->trieWalker;
	w.moveBack();
	return 0;
}

class PatternPrinter {
	BELPrintContext &ctxt;
public:
	PatternPrinter(BELPrintContext &pc) : ctxt(pc) {}
	// i'm not really sure if there's a suitable function already for these needs
	// so i added this one.
    void printPatternData(std::ostream &os, const BTND_PatternData &pd)
	{
		switch(pd.which()) {
		case BTND_Pattern_None_TYPE:
			os << "BTND_Pattern_None_TYPE";
			break;
			//abort();
#define CASEPD(x) case BTND_Pattern_##x##_TYPE: boost::get<BTND_Pattern_##x>(pd).print(os, ctxt); break;
		CASEPD(Token)
		CASEPD(Punct)
		CASEPD(CompoundedWord)
		CASEPD(Number)
		CASEPD(Wildcard)
		CASEPD(Date)
		CASEPD(Time)
		CASEPD(DateTime)
#undef CASEPD
		default:
			AYLOG(ERROR) << "Unexpected pattern type\n";
		}
	}

    // only prints the vector itself yet.
	void operator()(const BTND_PatternDataVec& seq) {
		//return;
		std::cout << "[";
		for(BTND_PatternDataVec::const_iterator pi = seq.begin(); pi != seq.end();) {
			printPatternData(std::cout, *pi);
			if (++pi != seq.end()) std::cout << ", ";
		}
		std::cout << "]\n";
	}
};

static int bshf_stexpand( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->universe;
	BELTrie &trie = context->getTrie();
	ay::UniqueCharPool &stringPool = uni.getStringPool();

	BELPrintFormat fmt;
	BELPrintContext ctxt( trie, stringPool, fmt );
	PatternPrinter pp(ctxt);
	BELExpandReader<PatternPrinter> reader(pp, &trie, uni.getGlobalPools());
	reader.initParser(BELReader::INPUT_FMT_XML);

	ay::stopwatch totalTimer;

	std::string sin;
	if (in >> sin) {
		const char *fname = sin.c_str();
		//AYLOG(DEBUG) << "Loading " << fname;
		int numsts = reader.loadFromFile(fname);
		std::cout << numsts << " statements read. in "  << totalTimer.calcTime() << std::endl;
	} else {
		//AYLOG(ERROR) << "no filename";
	}
	return 0;
}

static int bshf_querytest( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->universe;
	//BELTrie &trie = uni.getBarzelTrie();
	//ay::UniqueCharPool &stringPool = uni.getStringPool();

	ay::stopwatch totalTimer;

	size_t num = 0;
	if (in >> num) {
		shell->getOutStream() << "performing " << num << " queries...\n";
		while (num--) {
			std::string str = "<query>mama myla 2 ramy</query>";
			std::stringstream ss;
			BarzerRequestParser rp(uni.getGlobalPools(), ss);
			rp.parse(str.c_str(), str.size());
		}
		shell->getOutStream() << "done in " << totalTimer.calcTime() << "\n";
	} else {
		//AYLOG(ERROR) << "no filename";
	}
	return 0;
}


/// end of specific shell routines
static const CmdData g_cmd[] = {
	CmdData( ay::Shell::cmd_help, "help", "get help on a barzer function" ),
	// CmdData( ay::Shell::cmd_set, "set", "sets parameters" ),
	CmdData( ay::Shell::cmd_exit, "exit", "exit all barzer shells" ),
	CmdData( ay::Shell::cmd_quit, "quit", "exit the current barzer shell" ),
	CmdData( ay::Shell::cmd_run, "run", "execute script(s)" ),
	//commented test to reduce the bloat
	CmdData( bshf_test, "test", "just a test" ),
	CmdData( (ay::Shell_PROCF)bshf_anlqry, "anlqry", "<filename> analyzes query set" ),
	CmdData( (ay::Shell_PROCF)bshf_dtaan, "dtaan", "data set analyzer. runs through the trie" ),
	CmdData( (ay::Shell_PROCF)bshf_inspect, "inspect", "inspects types as well as the actual content" ),
	CmdData( (ay::Shell_PROCF)bshf_lex, "lex", "tokenize and then classify (lex) the input" ),
	CmdData( (ay::Shell_PROCF)bshf_tokenize, "tokenize", "tests tokenizer" ),
	CmdData( (ay::Shell_PROCF)bshf_xmload, "xmload", "loads xml from file" ),
	CmdData( (ay::Shell_PROCF)bshf_tok, "tok", "token lookup by string" ),
	CmdData( (ay::Shell_PROCF)bshf_tokid, "tokid", "token lookup by string" ),
	CmdData( (ay::Shell_PROCF)bshf_emit, "emit", "emit [filename] emitter test" ),
	CmdData( (ay::Shell_PROCF)bshf_entid, "entid", "entity lookup by entity id" ),
	CmdData( (ay::Shell_PROCF)bshf_euid, "euid", "entity lookup by euid (tok class subclass)" ),
	//CmdData( (ay::Shell_PROCF)bshf_trieloadxml, "trieloadxml", "loads a trie from an xml file" ),
	CmdData( (ay::Shell_PROCF)bshf_setloglevel, "setloglevel", "set a log level (DEBUG/WARNINg/ERROR/CRITICAL)" ),
	CmdData( (ay::Shell_PROCF)bshf_trans, "trans", "tester for barzel translation" ),
	CmdData( (ay::Shell_PROCF)bshf_triestats, "triestats", "trie commands" ),
	CmdData( (ay::Shell_PROCF)bshf_trie, "trie", "trie commands" ),
	CmdData( (ay::Shell_PROCF)bshf_trls, "trls", "lists current trie node children" ),
	CmdData( (ay::Shell_PROCF)bshf_trcd, "trcd", "changes current trie node to the firm child by number" ),
	CmdData( (ay::Shell_PROCF)bshf_trcdw, "trcdw", "changes current trie node to the wildcard child by number" ),
	CmdData( (ay::Shell_PROCF)bshf_trup, "trup", "moves back to the parent trie node" ),
	CmdData( (ay::Shell_PROCF)bshf_spell, "spell", "tests hunspell in the current universe spell checker" ),
	CmdData( (ay::Shell_PROCF)bshf_stexpand, "stexpand", "expand and print all statements in a file" ),
	CmdData( (ay::Shell_PROCF)bshf_process, "process", "process an input string" ),
	CmdData( (ay::Shell_PROCF)bshf_querytest, "querytest", "peforms given number of queries" ),
};

ay::ShellContext* BarzerShell::mkContext() { 
	return new BarzerShellContext(*d_universe, d_universe->getSomeTrie() );
}

ay::ShellContext* BarzerShell::cloneContext() { 
	return new BarzerShellContext(*(dynamic_cast<BarzerShellContext*>( context )));
}

int BarzerShell::init()
{
	int rc = 0;
	if( !cmdMap.size() )
		rc = indexCmdDataRange(ay::Shell::CmdDataRange( ARR_BEGIN(g_cmd),ARR_END(g_cmd)));

	if( context ) 
		delete context;

	context = mkContext();

	((BarzerShellContext*)context)->trieWalker.loadChildren();
	return rc;
}

} // barzer namespace ends here
