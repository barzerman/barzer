#include <barzer_shell.h>
#include <barzer_server.h>
#include <ay_util_time.h>
#include <ay_translit_ru.h>

#include <barzer_emitter.h>
#include <barzer_el_xml.h>
#include <barzer_el_trie.h>
#include <barzer_el_parser.h>
#include <barzer_el_wildcard.h>
#include <barzer_el_analysis.h>
#include <barzer_autocomplete.h>
#include <ay/ay_logger.h>
#include <ay/ay_choose.h>
#include <algorithm>
#include <barzer_el_trie_shell.h>
#include <barzer_server_response.h>
#include <functional>
#include <boost/function.hpp>

#include <barzer_server_request.h>
#include "barzer_geoindex.h"
//
#include <sstream>
#include <fstream>
#include <string.h>



namespace barzer {

namespace {
bool is_all_digits( const char* s )
{
    for( const char* x = s; *x; ++x )
        if( !isdigit(*x) )
            return false;
    return true;
}

}
BarzerShellContext::BarzerShellContext(StoredUniverse& u, BELTrie& trie) :
		gp(u.getGlobalPools()),
		d_universe(&u),
		trieWalker(trie),
		d_trie(&trie),
		parser( u ),
        d_grammarId(0),
        streamerModeFlags(streamerModeFlags_bits)
{
	barz.setUniverse(&u);
#define NAME_MODE_BIT( x ) streamerModeFlags.name( BarzStreamerXML::BF_##x, #x )
    NAME_MODE_BIT(NOTRACE);
    NAME_MODE_BIT(USERID);
    NAME_MODE_BIT(ORIGQUERY);
    NAME_MODE_BIT(QUERYID);
#undef NAME_MODE_BIT
}

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
static int bshf_srvroute( BarzerShell* shell, char_cp cmd, std::istream& in )
{

	// BarzerShellContext * context = shell->getBarzerContext();
	GlobalPools& gp = shell->gp;

	ay::InputLineReader reader( in );
	QuestionParm qparm;
    std::ostream& os = std::cerr;

    std::string question;
    while (reader.nextLine()&& reader.str.length())
        question.append(reader.str);
    char *q = strdup( question.c_str() );
    //const char* q = reader.str.c_str();
    int rc = request::route( gp, q, strlen(q), os );
    if( rc != request::ROUTE_ERROR_OK ) {
        os << " ERROR " << rc;
    } else
        os << " success ";
    os << "==============\n";
    free(q);
    
    return 0;
}
static int bshf_emit( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	const StoredUniverse &uni = context->getUniverse();
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

	request::proc_EMIT( reqEnv, globalPools, q.c_str() );
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

    std::string parmStr;
    size_t numIterations = 1;
    if( in >> parmStr ) {
        numIterations = atoi( parmStr.c_str() );
        if( !numIterations )
            numIterations = 1;
    }
	ay::InputLineReader reader( in );
	QuestionParm qparm;
    shell->syncQuestionParm(qparm);

	std::ostream& outFp = shell->getOutStream() ;
	while( reader.nextLine() && reader.str.length() ) {
		const char* q = reader.str.c_str();

		/// tokenize
        for( size_t i =0; i< numIterations; ++i ) {
		    parser.tokenize_only( barz, q, qparm );
		    const TTWPVec& ttVec = barz.getTtVec();
            if( !i ) {
		        outFp << "Tokens:" << ttVec << std::endl;
            }
		    /// classify tokens in the barz
		    parser.lex_only( barz, qparm );
            if( !i ) {
                const CTWPVec& ctVec = barz.getCtVec();
                outFp << "Classified Tokens:\n" << ctVec << std::endl;
            }
        }
	}
	return 0;
}

static int bshf_guesslang(BarzerShell *shell, char_cp cmd, std::istream& in)
{
	BarzerShellContext *context = shell->getBarzerContext();
	GlobalPools& pools = context->getUniverse().getGlobalPools();

	ay::InputLineReader reader(in);
	while (reader.nextLine() && reader.str.length())
	{
		const std::string& str = reader.str;

		std::vector<std::pair<int, double> > probs;
		ay::evalAllLangs(pools.getUTF8LangMgr(), pools.getASCIILangMgr(), str.c_str(), probs);

		for (size_t i = 0; i < probs.size(); ++i)
		{
			const char *lang = ay::StemWrapper::getValidLangString(probs[i].first);
			shell->getOutStream() << (lang ? lang : "unknown lang")
					<< " score: "
					<< probs.at(i).second
					<< std::endl;
		}

		shell->getOutStream() << std::endl;
	}
	return 0;
}

static int bshf_wordMeanings(BarzerShell *shell, char_cp cmd, std::istream& in)
{
	BarzerShellContext *context = shell->getBarzerContext();
	GlobalPools& pools = context->getUniverse().getGlobalPools();
	const MeaningsStorage&mst = context->getUniverse().meanings();

	ay::InputLineReader reader(in);
	while (reader.nextLine() && reader.str.length())
	{
		const std::string& str = reader.str;

		const WordMeaningBufPtr buf = mst.getMeanings(pools.internalString_getId(str.c_str()));
		shell->getOutStream() << "got " << buf.second << " meanings:\n";
		for (size_t i = 0; i < buf.second; ++i)
		{
			const char *mstr = pools.internalString_resolve_safe(buf.first[i].id);
			shell->getOutStream() << mstr << " (prio " << static_cast<int>(buf.first[i].prio) << ")\n";
		}

		shell->getOutStream() << std::endl;
	}
	return 0;
}

static int bshf_listMeaning(BarzerShell *shell, char_cp cmd, std::istream& in)
{
	BarzerShellContext *context = shell->getBarzerContext();
	GlobalPools& pools = context->getUniverse().getGlobalPools();
	const MeaningsStorage&mst = context->getUniverse().meanings();

	ay::InputLineReader reader(in);
	while (reader.nextLine() && reader.str.length())
	{
		const std::string& str = reader.str;

		const MeaningSetBufPtr buf = mst.getWords(pools.internalString_getId(str.c_str()));
		shell->getOutStream() << "got " << buf.second << " words in meaning:\n";
		for (size_t i = 0; i < buf.second; ++i)
		{
			const char *mstr = pools.internalString_resolve_safe(buf.first[i]);
			shell->getOutStream() << mstr << std::endl;
		}

		shell->getOutStream() << std::endl;
	}
	return 0;
}

static int bshf_allMeanings(BarzerShell *shell, char_cp cmd, std::istream& in)
{
	BarzerShellContext *context = shell->getBarzerContext();
	GlobalPools& pools = context->getUniverse().getGlobalPools();
	const MeaningsStorage &mst = context->getUniverse().meanings();

	const M2WDict_t& dict = mst.getMeaningsToWordsDict();

	for (M2WDict_t::const_iterator i = dict.begin(), end = dict.end();
			i != end; ++i)
		shell->getOutStream() << pools.internalString_resolve_safe(i->first)
				<< " (" << i->second.size() << " words)\n";

	shell->getOutStream() << std::endl;

	return 0;
}

static int bshf_findEntities(BarzerShell *shell, char_cp cmd, std::istream& in)
{
	BarzerShellContext *context = shell->getBarzerContext();
	const auto& uni = context->getUniverse();
	const auto& geo = *uni.getGeo();
	
	shell->getOutStream() << "the format is:\n"
			<< "longitude latitude distance class subclass\n"
			<< "where longitude and latitude are doublee, "
				"class and subclass can be negative to disable check"
			<< std::endl;
	
	ay::InputLineReader reader(in);
	while (reader.nextLine() && reader.str.length())
	{
		std::istringstream istr(reader.str);
		double lon, lat, dist;
		int64_t ec, esc;
		istr >> lon >> lat >> dist >> ec >> esc;
		
		shell->getOutStream() << "entities (" << ec << "; " << esc << ") near (" << lon << ", " << lat << "):" << std::endl;
		std::vector<uint32_t> ents;
		if (ec >= 0 && esc >= 0)
			geo.findEntities(ents, BarzerGeo::Point_t(lon, lat), EntClassPred(ec, esc, uni), dist);
		else
			geo.findEntities(ents, BarzerGeo::Point_t(lon, lat), DumbPred(), dist);
		
		shell->getOutStream() << "found " << ents.size() << " entities:" << std::endl << "{\n";
		for(size_t i = 0; i < ents.size(); ++i)
		{
			auto ent = uni.getGlobalPools().getDtaIdx().getEntById(ents[i]);
			if (!ent)
			{
				shell->getOutStream() << "\tinvalid entity " << ents[i] << std::endl;
				continue;
			}
			shell->getOutStream() << "\t" << ent->getClass() << " " << ent->getSubclass() << std::endl;
		}
		shell->getOutStream() << "}" << std::endl;
	}
}

static int bshf_tokenize( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;
	QParser& parser = context->parser;
	const StoredUniverse &uni = context->getUniverse();

	ay::InputLineReader reader( in );
	QuestionParm qparm;
    shell->syncQuestionParm(qparm);
	while( reader.nextLine() && reader.str.length() ) {
		barz.tokenize( uni.getTokenizerStrategy(), parser.tokenizer, reader.str.c_str(), qparm );
		const TTWPVec& ttVec = barz.getTtVec();
		shell->getOutStream() << ttVec << std::endl;
	}
	return 0;
}

static int bshf_tokenize_old( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;
	QParser& parser = context->parser;

	ay::InputLineReader reader( in );
	QuestionParm qparm;
    shell->syncQuestionParm(qparm);
	while( reader.nextLine() && reader.str.length() ) {
		barz.tokenize( parser.tokenizer, reader.str.c_str(), qparm );
		const TTWPVec& ttVec = barz.getTtVec();
		shell->getOutStream() << ttVec << std::endl;
	}
	return 0;
}

static int bshf_soundslike(BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	const StoredUniverse &uni = context->getUniverse();
	const GlobalPools& gp = uni.getGlobalPools();
	const BZSpell* bzSpell = uni.getBZSpell();

	if( !bzSpell ) {
		std::cerr << "bzspell is NULL\n";
		return 0;
	}
	ay::InputLineReader reader( in );

	while( reader.nextLine() && reader.str.length() ) {
		std::string out;
		bzSpell->getEnglishSL().transform(reader.str.c_str(), reader.str.size(), out);
		std::cout << "result: " << out << std::endl;
	}
	return 0;
}

static int bshf_bzspell( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	const StoredUniverse &uni = context->getUniverse();
	const GlobalPools& gp = uni.getGlobalPools();
	const BZSpell* bzSpell = uni.getBZSpell();

	if( !bzSpell ) {
		std::cerr << "bzspell is NULL\n";
		return 0;
	}
	ay::InputLineReader reader( in );

	while( reader.nextLine() && reader.str.length() ) {
		const char*  word = reader.str.c_str();
		uint32_t origStrid = 0xffffffff;
		int isUsersWord =  bzSpell->isUsersWord( origStrid, word ) ;
		if( isUsersWord ) {
			std::cerr << "this is a valid " << ( isUsersWord == 1 ? "user's " : "secondary " ) << "word\n";
			continue;
		}
		uint32_t strId = bzSpell->getSpellCorrection( word, true );
		if( strId != 0xffffffff ) {
			std::cerr << "corrected to " << std::hex << strId << ":";
			const char* corrected = gp.string_resolve( strId ) ;
			if( corrected ) {
				std::cerr << corrected << "\n";
			} else {
				std::cerr << "<invalid id>\n";
			}
		} else {
			std::cerr << "cannot correct\n";
		}
	}
	return 0;
}

static int bshf_bzstem( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	const StoredUniverse &uni = context->getUniverse();
	const BZSpell* bzSpell = uni.getBZSpell();

	if( !bzSpell ) {
		std::cerr << "bzspell is NULL\n";
		return 0;
	}
    bool stemOnly = !strcmp(cmd,"stem");

	ay::InputLineReader reader( in );

	while( reader.nextLine() && reader.str.length() ) {
		const char*  word = reader.str.c_str();
		std::string stem;

        if( stemOnly ) {
            std::stringstream sstr(word);
            std::string tmp;
            std::cerr << "STEM:";
            while( sstr >> tmp ) {
                stem.clear();
                bool stemSuccess = bzSpell->stem( stem, tmp.c_str() );
                std::cerr << ( stemSuccess ? stem : tmp ) << " ";
            }
            std::cerr << "<<<< ed of stem\n";

        } else {
            uint32_t strId = bzSpell->getStemCorrection( stem, word );
            std::cerr << "stemmed to :" << stem << ":" << std::hex << strId << ":" << std::endl;
        }
	}
	return 0;
}

namespace { /////
struct TopicAnalyzer {
    QParser& parser;
    std::ostream& fp;

	BELPrintFormat fmt;
    BELPrintContext printCtxt;

    TopicAnalyzer( QParser& p, std::ostream& os, const BELTrie& trie ) :
        parser(p), fp(os) ,
        printCtxt( trie, p.getUniverse().getStringPool(), fmt )

    {}
    void operator() ( const BTMIterator& nb ) {
    }
};

} /// end of anon namespace

static int bshf_greed( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;

	QParser parser( (context->getUniverse()) );

	const StoredUniverse &uni = context->getUniverse();
	BarzStreamerXML bs(barz, context->getUniverse());

    const BELTrie* trie=  &(context->getTrie());

    std::ostream& outFP = shell->getOutStream() ;
    TopicAnalyzer topicAnalyzer(parser, outFP, *trie );
    MatcherCallbackGeneric<TopicAnalyzer> cb(topicAnalyzer);


	ay::InputLineReader reader( in );
	while( reader.nextLine() && reader.str.length() ) {
	    QuestionParm qparm;
        shell->syncQuestionParm(qparm);

		const char* q = reader.str.c_str();
		outFP << "lexing: " << q << "\n";
		parser.lex( barz, q, qparm );
		outFP << "lexed. printing\n";
		const CTWPVec& ctVec = barz.getCtVec();
		outFP << "Classified Tokens: {" << ctVec << "}" << std::endl;
		bs.print(outFP);

		// << ttVec << std::endl;

        BarzelMatcher barzelMatcher( uni, *trie );
        BarzelBeadChain& beadChain = barz.getBeads();
        if( beadChain.lst.empty() ) {
            std::cerr << "no beads\n";
        }
        barzelMatcher.get_match_greedy( cb, beadChain.lst.begin(), beadChain.lst.end(), false );
	}
    return 0;
}

static int bshf_autoc( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;

	QParser parser( (context->getUniverse()) );

    BestEntities bestEnt;
	AutocStreamerJSON autocStreamer(bestEnt, context->getUniverse());

    // const BELTrie* trie=  &(context->getTrie());

    std::ostream& outFP = shell->getOutStream() ;
	std::string fname;
	QuestionParm qparm;
    shell->syncQuestionParm(qparm);

	//std::ostream *ostr = &(outFP);
	std::ofstream ofile;

    size_t numIterations = 1;
	if (in >> fname) {
        if( is_all_digits(fname.c_str()) ) {
            numIterations = atoi( fname.c_str() );
        } else {
            ofile.open(fname.c_str());
            //ostr = &ofile;
        }
	}

	ay::InputLineReader reader( in );
    qparm.isAutoc = true;
	while( reader.nextLine() && reader.str.length() ) {
		const char* q = reader.str.c_str();

        /// this will be invoked for every qualified node
	    ay::stopwatch localTimer;
        for( size_t i = 0; i< numIterations; ++i ) {
            BarzerAutocomplete autoc( barz, context->getUniverse(), qparm, outFP );
            autoc.parse(q);
            barz.clearWithTraceAndTopics();
        }
        std::cerr << std::dec << numIterations << " iterations done in " << localTimer.calcTime() << " seconds\n";
	}
    return 0;
}

static int bshf_smf( BarzerShell* shell, char_cp cmd, std::istream& in )
{
    BarzerShellContext * context = shell->getBarzerContext();

	std::vector< std::string > parm;
    std::ostream& fp = shell->getOutStream() ;
    std::string tmp;
	while( in >>tmp )  
        parm.push_back( tmp );
    if( parm.size() == 2 ) { // NAME VAL
        bool val = (parm[1] != "OFF" );
        context->streamerModeFlags.set( parm[0], val );
        fp << "set " << parm[0] << " to " << val << std::endl;
    } else if( parm.size() ==1 ) { // NAME 
        if( parm[0]== "CLEAR" ) {
            context->streamerModeFlags.theBits().clear();
            fp << "cleared all bits" << std::endl;
        } else {
            fp << parm[0] << "=" << context->streamerModeFlags.check( parm[0] ) << std::endl;
        }
    } else { // no parameters specified
        fp << context->streamerModeFlags;
    }
    return 0;
}

static int bshf_process( BarzerShell* shell, char_cp cmd, std::istream& in )
{
        BarzerShellContext * context = shell->getBarzerContext();
        if( !context || !context->isUniverseValid() ) {
                std::cerr << "barzer shell FATAL - context oruniverse is NULL\n";
                return 0;
        }
        Barz& barz = context->barz;
        // Barz barz;
        QParser parser( (context->getUniverse()) );

        BarzStreamerXML bs(barz, context->getUniverse());
        bs.setWholeMode( context->streamerModeFlags.theBits() );

        std::string fname;

        std::ostream *ostr = &(shell->getOutStream());
        std::ofstream ofile;

    size_t numIterations = 1;
        if (in >> fname) {
        if( is_all_digits(fname.c_str()) ) {
            numIterations = atoi( fname.c_str() );
        } else {
            ofile.open(fname.c_str());
            ostr = &ofile;
        }
        }
        ay::InputLineReader reader( in );

        QuestionParm qparm;
        shell->syncQuestionParm(qparm);

        ay::stopwatch totalTimer;
        while( reader.nextLine() && reader.str.length() ) {
            ay::stopwatch localTimer;
                const char* q = reader.str.c_str();
                *ostr << "parsing: " << q << "\n";

        for( size_t i = 0; i< numIterations; ++i ) {
            parser.parse( barz, q, qparm );
            if( !i ){
                *ostr << "parsed. printing\n";
                bs.print(*ostr);
            }
            barz.clearWithTraceAndTopics();
        }

        std::cerr << numIterations << " iterations done in " << localTimer.calcTime() << " seconds\n";
                // << ttVec << std::endl;
        }
        std::cerr << "All done in " << totalTimer.calcTime() << " seconds\n";
        return 0;
}

static int bshf_anlqry( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	Barz& barz = context->barz;
	// Barz barz;
	QParser& parser = context->parser;

	BarzStreamerXML bs(barz, context->getUniverse());
	std::string fname;

	std::ostream *ostr = &(shell->getOutStream());
	std::ofstream ofile;

	ay::InputLineReader reader( in );
	if (in >> fname) {
		ofile.open(fname.c_str());
		ostr = &ofile;
	}

	QuestionParm qparm;
    shell->syncQuestionParm(qparm);
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
	*ostr << "\n" << numQueries << " (" << numSemanticalBarzes << " semantical) queries processed read. in "  << totalTimer.calcTime() << std::endl;
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
		uni(context->getUniverse()),
		walker(context->trieWalker),
		trie(context->getTrie()),
		curTrieNode(walker.getCurrentNode()),
		stringPool( uni.getStringPool())
	{}

};

} // anon namespace ends

static int bshf_universe( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	std::string tmp;
    std::ostream& outFp = shell->getOutStream() ;

    const GlobalPools & gp = shell->gp;
    in >> tmp;
    const GlobalPools::UniverseMap& uniMap = gp.getUniverses();
    for( GlobalPools::UniverseMap::const_iterator i = uniMap.begin(); i!= uniMap.end(); ++i ) {
        const StoredUniverse* u = i->second;
        if( u ) {
            
            if( !tmp.length() || strstr( u->userName().c_str(), tmp.c_str() ) ) 
                outFp << u->userName() << ":" << u->getUserId() << std::endl;
        }
    }
    return 0;
}
static int bshf_userstats( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	// BarzerShellContext * context = shell->getBarzerContext();
	std::string tmp;
    std::ostream& outFp = shell->getOutStream() ;

    const GlobalPools & gp = shell->gp;
	if( in >> tmp ) {
        uint32_t userId = atoi(tmp.c_str()) ;
        std::string modeStr ;
        bool doTrieStats = false;
        if( in >> modeStr ) {
            if( strchr( modeStr.c_str(), 'f' ) )
                doTrieStats = true;
        }

        const StoredUniverse* uni = gp.getUniverse( userId ) ;
        if( uni ) {
            const TheGrammarList& trieList = uni->getTrieList();
            if( trieList.size() ) {
                int n = 0;
                for( TheGrammarList::const_iterator i = trieList.begin(); i!= trieList.end(); ++i )
                {
                    const BELTrie& trie = i->trie();
                    outFp << ">> Trie " << ++n << ":" << "(" << trie.getTrieClass()   << "|" << trie.getTrieId()  << ')' << std::endl;
                    if( doTrieStats ) {
                        BarzelTrieTraverser_depth trav( trie.getRoot(), trie );
                        BarzelTrieStatsCounter counter;
                        trav.traverse( counter, trie.getRoot() );
                        std::cerr << counter << std::endl;
                    }
                }
            } else {
                outFp << "User has no tries\n";
            }
        } else {
            std::cerr << "No valid universe for user id " << userId << "\n";

        }
    } else {
        outFp << "User ids:";
        const GlobalPools::UniverseMap&  uniMap = gp.getUniverseMap();
        for( GlobalPools::UniverseMap::const_iterator i = uniMap.begin(); i!= uniMap.end(); ++i ) {
            outFp << i->first << ',';
        }
        outFp << std::endl;
        std::cerr << "to inspect individual universe say: userstats <user id>\n";
    }

    return 0;
}

static int bshf_triestats( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	ShellState sh( shell, cmd, in );

    std::ostream& outFP = shell->getOutStream() ;
    std::string trieClass;

    StoredUniverse &uni = context->getUniverse();
	GlobalPools &globalPools = context->getGLobalPools();
	if (in >> trieClass ) {
		std::string trieId;
        if( in >> trieId ) {
            BELTrie* trie = globalPools.getTrie( trieClass.c_str(), trieId.c_str() );
            if( trie ) {
                context->setTrie( trie );
                context->trieWalker.setTrie( trie );
                outFP << "SETTING TRIE: \"" << trieClass << "\":\"" << trieId << "\"\n";
            } else {
                outFP << "Trie not found\n";
            }
            BarzelTrieTraverser_depth trav( sh.trie.getRoot(), shell->getBarzerContext()->getTrie() );
            BarzelTrieStatsCounter counter;
            trav.traverse( counter, sh.curTrieNode );
            outFP << counter << std::endl;
            return 0;
        }

	}
    const TheGrammarList& grammarList = uni.getTrieList();
    for( TheGrammarList::const_iterator i = grammarList.begin(); i!= grammarList.end(); ++i ) {
        const BELTrie& theTrie = i->trie();
        outFP << "TRIE [" << theTrie.getTrieClass() << ":" << theTrie.getTrieId() << "]" << std::endl;
        BarzelTrieTraverser_depth trav( theTrie.getRoot(), theTrie );
        BarzelTrieStatsCounter counter;
        trav.traverse( counter, theTrie.getRoot() );
        outFP << counter << std::endl;
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

static int bshf_translit( BarzerShell* shell, char_cp cmd, std::istream& in )
{
    bool engToRus = ( !cmd || cmd[0] == 't'  );
	ay::InputLineReader reader( in );
    while( reader.nextLine() && reader.str.length() ) {
        const char* q = reader.str.c_str();
	    std::stringstream ss(q);
        std::string tmp ;
        while( ss >> tmp ) {
            for( std::string::iterator i = tmp.begin(); i!= tmp.end(); ++i) 
                *i = tolower(*i);
            std::string result;
            ay::tl::en2ru(tmp.c_str(), tmp.length(), result);
            std::cout << result << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}
static int bshf_xtrans( BarzerShell* shell, char_cp cmd, std::istream& in )
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
	StoredUniverse &uni = context->getUniverse();
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
static int bshf_trieclear( BarzerShell* shell, char_cp cmd, std::istream& in )
{

	BarzerShellContext * context = shell->getBarzerContext();
	BELTrie* trie = context->getCurrentTriePtr();
	if( !trie ) {
		std::cerr << "current trie is NULL\n";
		return 0;
	} else {
		std::cerr << "clearing current trie\n";
		trie->clear();
		context->trieWalker.setTrie( trie );
	}
	return 0;
}
static int bshf_grammar( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	// GlobalPools &globalPools = context->getGLobalPools();
	std::string gramStr;
	if( in >> gramStr ) {
        int grammarId = atoi( gramStr.c_str() );

	    StoredUniverse &uni = context->getUniverse();

        const TheGrammarList& trieList = uni.getTrieList();

        int gi =0;
	    BELTrie* trie = 0;
        for( TheGrammarList::const_iterator i = trieList.begin(); i!= trieList.end(); ++i ) {
            if( gi == grammarId ) {
	            trie = const_cast<BELTrie*>(&(i->trie()));
            }
        }
        if(!trie ) {
            std::cerr << "grammar id " << grammarId << " not found\n";
        } else {
            context->setTrie( trie );
            context->trieWalker.setTrie( trie );

            std::cerr << "SETTING TRIE: \"" << trie->getTrieClass() << "\":\"" << trie->getTrieId()<< "\"\n";
        }
    } else {
        std::cerr << "current grammar is: " << context->d_grammarId << " for user "<< shell->getUser() << "\n";
    }
	return 0;
}
static int bshf_trieset( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext * context = shell->getBarzerContext();
	GlobalPools &globalPools = context->getGLobalPools();
	std::string trieClass, trieId;
	in >> trieClass >> trieId;
	BELTrie* trie = globalPools.getTrie( trieClass.c_str(), trieId.c_str() );
    if( trie ) {
        context->setTrie( trie );
        context->trieWalker.setTrie( trie );
        std::cerr << "SETTING TRIE: \"" << trieClass << "\":\"" << trieId << "\"\n";
    } else {
        std::cerr << "trie not found\n";
    }
	return 0;
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

static int bshf_strid( BarzerShell* shell, char_cp cmd, std::istream& in )
{
    BarzerShellContext * context = shell->getBarzerContext();

	const GlobalPools &gp = context->getGLobalPools();
	std::string tmp;
    uint32_t strId = 0xffffffff;

    while( in >> strId ) {
    const char* str = gp.string_resolve( strId ) ;
    const char* internalStr = gp.internalString_resolve( strId ) ;
    std::cerr << std::hex << strId << ":" << ( str ? str : "<null>" ) << ":" << (internalStr ? internalStr : "<null>" ) << std::endl;
    }
    return 0;
}
static int bshf_stexpand( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->getUniverse();
	BELTrie &trie = context->getTrie();
	ay::UniqueCharPool &stringPool = uni.getStringPool();

	BELPrintFormat fmt;
	BELPrintContext ctxt( trie, stringPool, fmt );
	PatternPrinter pp(ctxt);
	BELExpandReader<PatternPrinter> reader(pp, &trie, uni.getGlobalPools(), 0 );
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

static int bshf_instance( BarzerShell* shell, char_cp cmd, std::istream& in )
{
    BarzerShellContext * context = shell->getBarzerContext();

	const GlobalPools &gp = context->getGLobalPools();
    const GlobalPools::UniverseMap& uniMap = gp.getUniverseMap();

    std::ostream& outFP = shell->getOutStream() ;

    BarzelTrieStatsCounter instanceCounter;
    size_t numUsers=0;

    uint32_t fromUserId = 0, toUserId = 0xffffffff;
    uint32_t tmp =0xffffffff;
    if( in >> tmp ) {
        fromUserId = tmp;

        if( in >> tmp && tmp>= fromUserId ) 
            toUserId = tmp;
        else
            toUserId = fromUserId;
    }

    size_t numUniverses = 0;
    for( GlobalPools::UniverseMap::const_iterator i = uniMap.begin(); i!= uniMap.end(); ++i, ++numUsers ) {
        if( !(i->second) )
            continue;
        const StoredUniverse& uni = *(i->second);
        if( !(uni.getUserId() >= fromUserId && uni.getUserId() <= toUserId ))  {
            continue;
        }
        ++numUniverses;
        const TheGrammarList& grammarList = uni.getTrieList();
        BarzelTrieStatsCounter userCounter;

        for( TheGrammarList::const_iterator gi = grammarList.begin(); gi!= grammarList.end(); ++gi ) {
            const BELTrie& theTrie = gi->trie();
            // outFP << "TRIE [" << theTrie.getTrieClass() << ":" << theTrie.getTrieId() << "]" << std::endl;
            BarzelTrieTraverser_depth trav( theTrie.getRoot(), theTrie );
            BarzelTrieStatsCounter counter;
            trav.traverse( counter, theTrie.getRoot() );
            // outFP << counter << std::endl;
            userCounter.add( counter );
        }
        userCounter.print( (outFP << "USER:\t" << i->first << "\t" << std::setw(16) << uni.userName() << "\t"), "\t")  << "\n";

        instanceCounter.add( userCounter );
    }
    if( numUniverses > 1 )
        instanceCounter.print( (outFP << "#usrs:\t" << numUniverses << "\t" << std::setw(16)  << "Total" << "\t"), "\t" ) << std::endl;

    return 0;
}
static int bshf_env( BarzerShell* shell, char_cp cmd, std::istream& in )
{
    const char* name = 0, *value = 0;
    std::string n, v;
	if(in >> n) name=n.c_str();
    if(in >> v) value=v.c_str();

    std::ostream& outFP = shell->getOutStream() ;
    if( value ) {
        shell->shellEnv.set( outFP, name, value );
    } else {
        shell->shellEnv.get( outFP, name );
    }
    return 0;
}

static int bshf_entlist( BarzerShell* shell, char_cp cmd, std::istream& in )
{
        StoredEntityPool entpool = shell->getBarzerContext()->obtainDtaIdx()->entPool;
        std::ostream& out = shell->getOutStream() ;
        GlobalPools& gp = shell->getBarzerContext()->getGLobalPools();
        
        size_t amount = entpool.getNumberOfEntities();
        out << "Number of entities: " << amount << std::endl;
        const StoredEntity* e = 0;
        for (size_t i = 0; i < amount; i++)
        {
                e = entpool.getEntByIdSafe(i);  
                if (e->euid.getClass().ec < 100)
                        out << e->euid.getClass() << "\t" << gp.internalString_resolve(e->euid.getTokId()) << std::endl;//<<"/t"<<  gp.string_resolve(e->euid.getTokId())<<std::endl;
        }
        return 0;
}


static int bshf_user( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	uint32_t uid = 0;
    std::ostream& outFP = shell->getOutStream() ;
	if (in >> uid) {
		int rc = shell->setUser( uid, false ) ;
		if( rc )
			outFP << "error=" << rc << " setting user to " << uid << "\n";
		else
			outFP << "user is set to " << uid << "\n";
	} else {
        outFP << "Current user is " << shell->getUser() << "\n";

	    BarzerShellContext *context = shell->getBarzerContext();
	    StoredUniverse &uni = context->getUniverse();
        const TheGrammarList& trieList = uni.getTrieList();
        size_t numGrammars = 0;
        for( TheGrammarList::const_iterator i = trieList.begin(); i!= trieList.end(); ++i ) {
            const BELTrie* trie = &(i->trie());
            outFP << numGrammars << " [" << trie->getTrieClass() << ":" << trie->getTrieId() << "]" << std::endl;

            ++numGrammars ;
        }
        outFP << "**** total " << numGrammars << std::endl;
    }
	return 0;
}

static int bshf_querytest( BarzerShell* shell, char_cp cmd, std::istream& in )
{
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->getUniverse();
	//BELTrie &trie = uni.getBarzelTrie();
	//ay::UniqueCharPool &stringPool = uni.getStringPool();

	ay::stopwatch totalTimer;

	size_t num = 0;
	uint32_t userId = 0; // maybe add something that gets current user id from context
	if (in >> num) {
		shell->getOutStream() << "performing " << num << " queries...\n";
		while (num--) {
			std::string str = "<query>mama myla 2 ramy</query>";
			std::stringstream ss;
			BarzerRequestParser rp(uni.getGlobalPools(), ss, userId);
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
	CmdData( (ay::Shell_PROCF)bshf_autoc, "autoc", "autocomplete" ),
	CmdData( (ay::Shell_PROCF)bshf_bzspell, "bzspell", "bzspell correction for the user" ),
	CmdData( (ay::Shell_PROCF)bshf_bzstem, "bzstem", "bz stemming correction for the user domain" ),
	CmdData( (ay::Shell_PROCF)bshf_env, "env", "set env parameter. usage: env [name [value]]" ),
	CmdData( (ay::Shell_PROCF)bshf_dtaan, "dtaan", "data set analyzer. runs through the trie" ),
	CmdData( (ay::Shell_PROCF)bshf_instance, "instance", "lists all users in the instance" ),
	CmdData( (ay::Shell_PROCF)bshf_inspect, "inspect", "inspects types as well as the actual content" ),
	CmdData( (ay::Shell_PROCF)bshf_grammar, "grammar", "sets trie for given grammar. use 'user' to list grammars" ),
	CmdData( (ay::Shell_PROCF)bshf_guesslang, "guesslang", "guess a language from a word/string" ),
	CmdData( (ay::Shell_PROCF)bshf_wordMeanings, "wordmeanings", "list meanings for a word" ),
	CmdData( (ay::Shell_PROCF)bshf_listMeaning, "listmeaning", "list contents of a meaning" ),
	CmdData( (ay::Shell_PROCF)bshf_allMeanings, "allmeanings", "get all meanings" ),
	CmdData( (ay::Shell_PROCF)bshf_findEntities, "findents", "find entities near a point" ),
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
	CmdData( (ay::Shell_PROCF)bshf_soundslike, "soundslike", "check the Soundslike heuristics for user words" ),
	CmdData( (ay::Shell_PROCF)bshf_bzstem, "stem", "bz stemming" ),
	CmdData( (ay::Shell_PROCF)bshf_xtrans, "trans", "tester for barzel translation" ),
	CmdData( (ay::Shell_PROCF)bshf_translit, "translit", "transliterate english to russian (input must all lower case )" ),
	CmdData( (ay::Shell_PROCF)bshf_translit, "ru2en", "transliterate russian to english" ),
	CmdData( (ay::Shell_PROCF)bshf_triestats, "triestats", "trie commands" ),
	CmdData( (ay::Shell_PROCF)bshf_trie, "trie", "trie commands" ),
	CmdData( (ay::Shell_PROCF)bshf_trieclear, "trieclear", "out the current trie (trieset to set current trie)" ),
	CmdData( (ay::Shell_PROCF)bshf_trieset, "trieset", " [trieclass] [trieid] - sets current trie trie" ),
	CmdData( (ay::Shell_PROCF)bshf_trls, "trls", "lists current trie node children" ),
	CmdData( (ay::Shell_PROCF)bshf_trcd, "trcd", "changes current trie node to the firm child by number" ),
	CmdData( (ay::Shell_PROCF)bshf_trcdw, "trcdw", "changes current trie node to the wildcard child by number" ),
	CmdData( (ay::Shell_PROCF)bshf_trup, "trup", "moves back to the parent trie node" ),
	CmdData( (ay::Shell_PROCF)bshf_srvroute, "srvroute", "tests server queries and routes it same way server mode would" ),
	CmdData( (ay::Shell_PROCF)bshf_smf, "smf", "streamer mode flag: smf [ NAME [ON|OFF] ]" ),
	CmdData( (ay::Shell_PROCF)bshf_stexpand, "stexpand", "expand and print all statements in a file" ),
	CmdData( (ay::Shell_PROCF)bshf_strid, "strid", "resolve string id (usage strid id)" ),
	CmdData( (ay::Shell_PROCF)bshf_process, "process", "process an input string" ),
	CmdData( (ay::Shell_PROCF)bshf_process, "proc", "process an input string" ),
	CmdData( (ay::Shell_PROCF)bshf_process, "проц", "process an input string" ),
	CmdData( (ay::Shell_PROCF)bshf_greed, "greed", "non rewriting full match" ),
	CmdData( (ay::Shell_PROCF)bshf_querytest, "querytest", "peforms given number of queries" ),
	CmdData( (ay::Shell_PROCF)bshf_userstats, "userstats", "trie stats for a given user" ),
	CmdData( (ay::Shell_PROCF)bshf_user, "user", "sets current user by user id" ),
	CmdData( (ay::Shell_PROCF)bshf_universe, "universe", "looks up user id from universe name" ),
	CmdData( (ay::Shell_PROCF)bshf_entlist, "entlist", "prints all known entities")      
};

ay::ShellContext* BarzerShell::mkContext() {
	StoredUniverse& u = gp.produceUniverse(d_uid);

	return new BarzerShellContext( u, u.getSomeTrie() );
}

ay::ShellContext* BarzerShell::cloneContext() {
	return new BarzerShellContext(*(dynamic_cast<BarzerShellContext*>( context )));
}
int BarzerShell::setUser( uint32_t uid, bool forceCreate )
{
	if( !context ) {
		AYLOG(ERROR) << "null context\n";
		return 666;
	}
	StoredUniverse * u = gp.getUniverse(uid);
	if( !u ) {
		if( forceCreate )
			u = &( gp.produceUniverse(uid) );
		else
			return 1;
	}
	BarzerShellContext* bctxt = getBarzerContext();
	bctxt->setUniverse(u);
	bctxt->barz.setUniverse(u);
	d_uid = uid;
	return 0;
}
int BarzerShell::init( )
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

/// prints result of the setting to the stream
void BarzerShellEnv::set( std::ostream& fp, const char* n, const char* v )
{
    if( !n ) {
        fp << "ERROR: no name specified\n";
        return;
    }
    if( !v ) {
        fp << "ERROR: no valid value passed\n";
        return;
    }
    switch( n[0] ) {
    case 's':
        if( !strcasecmp(n,"stem") ) {
            if( !strncasecmp(v,"ag",2) ) {
                stemMode = QuestionParm::STEMMODE_AGGRESSIVE;
                fp << "stemMode set to Aggressive\n";
            } else
            if( !strncasecmp(v,"no",2) ) {
                stemMode = QuestionParm::STEMMODE_NORMAL;
                fp << "stemMode set to Normal\n";
            } else {
                fp << "valid options for are normal and aggressive\n";
            }
        }
        break;
    }
}

void BarzerShell::syncQuestionParm(QuestionParm& qparm )
{
    qparm.stemMode = shellEnv.stemMode;
}

// when n == 0 prints all valid settings
std::ostream&  BarzerShellEnv::get( std::ostream& fp, const char* n) const
{
    static const char * names[] = {
        "stem"
    };
    if( n ) {
        switch( n[0] ) {
        case 's': if( !strcasecmp(n,"stem") ) fp << "stem\t:" << ( stemMode == QuestionParm::STEMMODE_NORMAL ? "normal" : "aggressive" ) << std::endl; break;
        }
    }  else {
        for( const char* *s = ARR_BEGIN(names); s!= ARR_END(names); ++s )
            get(fp,*s);
    }
    return fp;
}

} // barzer namespace ends here
