/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_emitter.h>
#include <barzer_el_parser.h>
#include <barzer_el_xml.h>
#include <barzer_universe.h>
#include <barzer_server_response.h>
#include <fstream>

#include <ay/ay_logger.h>
#include <ay_util_time.h>
namespace barzer {

BELParser::BELParser(BELReader* r ) : reader(r) {}

const BarzelEvalNode* BELParser::getProcByName( uint32_t strId ) const
{
	const BELTrie& trie = reader->getTrie();
	return trie.getProcs().getEvalNode( strId );
}

const char* BELReader::getTrieName( ) const
{
    return getGlobalPools().internalString_resolve_safe( getTrie().getTrieId_strId() );
}
const char* BELReader::getTrieClassName( ) const
{
    return getGlobalPools().internalString_resolve_safe( getTrie().getTrieClass_strId() );
}

const BELParseTreeNode* BELParser::getMacroByNameId( uint32_t i ) const
{
	const BELTrie& trie = reader->getTrie();
	return trie.getMacros().getMacro( i );
}

const BELParseTreeNode* BELParser::getMacroByName( const BELTrie& trie, const char* macroname ) const
{
    uint32_t macroNameId = reader->getGlobalPools().internalString_getId( macroname );
	return trie.getMacros().getMacro( macroNameId );
}
const BELParseTreeNode* BELParser::getMacroByName( const char* macroname ) const
{
	const BELTrie& trie = reader->getTrie();
    uint32_t macroNameId = reader->getGlobalPools().internalString_getId( macroname );
	return trie.getMacros().getMacro( macroNameId );
}

uint32_t BELParser::stemAndInternTmpText( const char* s, int len )
{
	StoredUniverse* curUni = reader->getCurrentUniverse();
	if( !curUni )
		curUni = &(getGlobalPools().produceUniverse(0));
    //int lang = LANG_UNKNOWN;
    int lang = curUni->getDefaultLang();
	//std::cout << "parse lang:" << lang << "\n";
	uint32_t strId = curUni->stemAndIntern( lang, s, len, &(reader->getTrie()) );
    
    if( reader->isLiveCommandMode() ) {
        if( BZSpell* bzSpell = curUni->getBZSpell() )
            bzSpell->produceWordVariants(strId,lang);
    }
    return strId;
}

uint32_t BELParser::internVariable( const char* t )
{
	const char* beg = t;
	const char* end = strchr( t, '.' );
	std::string tmp;
	BELSingleVarPath vPath;
	for( ;end; ) {
		tmp.assign( beg, end-beg );
		vPath.push_back( internString_internal(tmp.c_str()) );
		beg = end+1;
		end = strchr( beg, '.' );
	}
	if( *beg ) {
		tmp.assign( beg );
		vPath.push_back( internString_internal(tmp.c_str()));
	}
	return reader->getTrie().getVarIndex().produceVarIdFromPathForTran(vPath);
}

uint32_t BELParser::addCompoundedWordLiteral( const char* alias )
{
	uint32_t aliasId = ( alias ? internString(LANG_UNKNOWN,alias,false,0).getStringId() : 0xffffffff );
	GlobalPools &gp = reader->getGlobalPools();
	uint32_t cwid = gp.getCompWordPool().addNewCompWordWithAlias( aliasId );
	StoredToken& sTok =  gp.getDtaIdx().addCompoundedToken(cwid);
	return sTok.tokId;
}

uint32_t BELParser::internString_internal( const char* t )
{
	return reader->getGlobalPools().internString_internal( t );
}
void BELReader::initCurrentUniverseToZero()
{
    if( !d_currentUniverse ) {
        d_currentUniverse = &(getGlobalPools().produceUniverse(0));
    }
}

StoredToken& BELParser::internString( int lang, const char* t, bool noSpell, const char* unstemmed)
{
	// here we may want to tweak some (nonexistent yet) fields in StoredToken
	// to reflect the fact that this thing is actually in the trie
    StoredUniverse* curUni = reader->getCurrentUniverse();
    return curUni->internString( lang, t, (noSpell ? 0: &(reader->getTrie()) ), unstemmed );
}

void BELParseTreeNode::print( std::ostream& fp, int depth ) const
{
	std::string pfx( depth*2, ' ');
	fp << pfx << "btndVar[" << btndVar.which() << "] {\n";
	++depth;
	for( ChildrenVec::const_iterator i = child.begin(); i!= child.end(); ++i ) {
		i->print( fp, depth );
	}
	fp << pfx << "}\n";
}


BELReader::BELReader( BELTrie* t, GlobalPools &g, std::ostream* errStream ) :
    trie(t) , parser(0), gp(g),
    numStatements(0) ,
    numSkippedStatements(0) ,
    numMacros(0) ,
    numProcs(0) ,
    numEmits(0),
    d_defaultExecMode(BELStatementParsed::EXEC_MODE_REWRITE),
    silentMode(false),
    d_trieSpellPriority(0),
    d_rulesetSpellPriority(0),
    d_spellPriority(0),
    d_liveCommand(false),
    inputFmt(INPUT_FMT_XML),
    d_currentUniverse(0),
    d_currentUser(0),
    d_curTrieId(g.internString_internal("")),
    d_curTrieClass(g.internString_internal("")),
    d_errStream(errStream? errStream: &(std::cerr)),
    d_maxEmitCountPerStatement(DEFMAX_EMIT_PER_STMT),
    d_maxEmitCountPerTrie(DEFMAX_EMIT_PER_SET),
    d_maxStatementsPerTrie(DEFMAX_STMT_PER_SET),
    d_respectLimits(true),
    d_noCanonicalNames(0)
{}
BELReader::BELReader( GlobalPools &g, std::ostream* errStream ) :
	trie(g.globalTriePool.produceTrie(g.internString_internal(""),g.internString_internal(""))) , parser(0), gp(g),
	numStatements(0) ,
	numSkippedStatements(0) ,
    numMacros(0) ,
    numProcs(0) ,
    numEmits(0),
    d_defaultExecMode(BELStatementParsed::EXEC_MODE_REWRITE),
    silentMode(false),
	d_trieSpellPriority(0),
    d_rulesetSpellPriority(0),
    d_spellPriority(0),
    d_liveCommand(false),
	inputFmt(INPUT_FMT_XML),
	d_trieIdSet(false),
    d_errStream(errStream? errStream: &(std::cerr)),
    d_maxEmitCountPerStatement(DEFMAX_EMIT_PER_STMT),
    d_maxEmitCountPerTrie(DEFMAX_EMIT_PER_SET),
    d_maxStatementsPerTrie(DEFMAX_STMT_PER_SET),
    d_noCanonicalNames(0)
{}

void BELReader::setTrie( uint32_t trieClass, uint32_t trieId )
{
	if( d_trieIdSet ) {
		trie = (gp.globalTriePool.produceTrie( d_curTrieClass, d_curTrieId ));
		computeImplicitTrieSpellPriority(d_curTrieClass, d_curTrieId);
	} else {
		trie = (gp.globalTriePool.produceTrie( trieClass, trieId ));
		computeImplicitTrieSpellPriority(trieClass,trieId);
	}
}

std::ostream& BELReader::printNode( std::ostream& fp, const BarzelTrieNode& node ) const
{
	BELPrintFormat fmt;
	BELPrintContext ctxt( *trie, gp.getStringPool(), fmt );
	return node.print( fp, ctxt );
}
void BELReader::addProc( uint32_t strId, const BELStatementParsed& sp )
{
    if (   sp.translation.child.size() == 0 ) 
    {
            std::ostream& os = sp.getErrStream() ;
           os << "<error type=\"WARNING\">Empty procedure ";
           xmlEscape(sp.getSourceName().c_str(), os) << ':' << sp.getStmtNumber()   << " is not permitted. Skipping.</error>\n";
    }
    
	int rc = getTrie().getProcs().generateStoredProc( strId, sp.translation );
	if( rc == BarzelProcs::ERR_OK ) {
		++numProcs;
	} else {
                std::ostream& os = sp.getErrStream() ;
                os << "<error type=\"WARNING\">Failed to load procedure in statement";
                xmlEscape(sp.getSourceName().c_str(), os) << ':' << sp.getStmtNumber()   << "</error>\n"; 
	}
	++numStatements;
}
void BELReader::addMacro( uint32_t macroNameId, const BELStatementParsed& sp )
{
	BELParseTreeNode& storedMacro = getTrie().getMacros().addMacro( macroNameId ) ;
	storedMacro = sp.pattern;
	++numMacros;
	++numStatements;
}

void BELReader::addStatement_imperative( const BELStatementParsed& sp, bool pre )
{
    trie->addImperative( sp, pre );
}
void BELReader::addStatement( const BELStatementParsed& sp )
{
    if( sp.isImperativeExec() )
    {
        addStatement_imperative( sp, sp.getExecMode() == BELStatementParsed::EXEC_MODE_IMPERATIVE_PRE );
        return;
    }
    if( !isOk_EmitCountPerTrie(numEmits) ) {
        std::ostream& os = sp.getErrStream() ;
        sp.getErrStream() << "<error type=\"ERROR\">";
        xmlEscape(sp.getSourceName().c_str(), os) << ':' << sp.getStmtNumber()  
                << " rejected: total number of emits for this statement set exceeds " << d_maxEmitCountPerTrie << "</error>"<<std::endl;

    }
    size_t emitPower = BELStatementParsed_EmitCounter(trie,d_maxEmitCountPerStatement).power( sp.pattern );

    if( d_respectLimits && emitPower>= d_maxEmitCountPerStatement ) {
        std::ostream& os = sp.getErrStream() ;
        sp.getErrStream() << "<error type=\"ERROR\">";
        xmlEscape(sp.getSourceName().c_str(), os) << ':' << sp.getStmtNumber()  << " rejected: number of emits for this statement is " 
                << emitPower << " and greater than " << d_maxEmitCountPerStatement<< "</error>"<<std::endl;   
       if( d_respectLimits )
            return;
    }

    numEmits += emitPower;


	BELParseTreeNode_PatternEmitter emitter( sp.pattern );
    emitter.reserveVecs();

	int i = 0, j = 0;
	ay::stopwatch totalTimer;
	const GlobalPools &gp = getGlobalPools();
	do {

		const BTND_PatternDataVec& seq = emitter.getCurSequence();
		const BELVarInfo& varInfo = emitter.getVarInfo();

		if( !seq.size() )
			continue;
		if( seq.size() > gp.getMaxAnalyticalModeMaxSeqLength() && gp.isAnalyticalMode() ) {
			continue;
		}
		if (seq.size() != varInfo.size()) {
			AYLOG(ERROR) << "Panic: varInfo.size != seq.size()";
		}

		j += seq.size();

		{ // adding path to the trie
			uint32_t tranId = 0xffffffff;
			BarzelTranslation* tran = trie->makeNewBarzelTranslation( tranId );
			if( !tran ) {
				AYLOG(ERROR) << "null translation returned\n";
			} else {
				trie->setTanslationTraceInfo( *tran, sp, i );
				if( tran->set(*trie, sp.translation) ) {
                    std::ostream& os = sp.getErrStream();
                    os << "<error type=\"INVALID TRANSLATION\">" << sp.getSourceName() << ':' << sp.getStmtNumber()  << "</error> " ;


                }
                if( sp.isTranUnmatchable() )
                    tran->makeUnmatchable = 1;
                tran->confidenceBoost = sp.getTranConfidenceBoost();
			}
			trie->addPath( sp, seq, tranId, varInfo, i, d_currentUniverse );
		}
		i++;
		//AYLOG(DEBUG) << "path added";
	} while( emitter.produceSequence() );
	// AYLOG(DEBUG) << i << " sequences(" <<  j << " nodes) produced in " << totalTimer.calcTime();

	++numStatements;
	// AYDEBUG(numStatements);
}


BELParser*  BELReader::initParser(InputFormat fmt )
{
	if( parser ) {
        
		if( fmt != inputFmt ) {
			inputFmt = fmt;
		} 
        delete parser;
        parser = 0;
	}
	if( fmt ==  INPUT_FMT_AUTO ) { // try to determine the format from file
		if( inputFileName.length() < 4 ) {
			std::cerr << "Fatal BELReader cannot deduce input format for file \"" << inputFileName << "\"\n";
			return 0;
		} else {
			std::cerr << "BELReader automatic format deduction not implemented yet\n";
			return 0;
		}
	}
	switch( fmt ) {
	case INPUT_FMT_XML: return ( (parser = new BELParserXML( this )) );
	case INPUT_FMT_BIN:
		std::cerr << "BELReader: binary format not implemented yet\n";
		return 0;
	case INPUT_FMT_BARZEL:
		std::cerr << "BELReader: barzel syntax not implemented yet\n";
		return 0;
	default:
		std::cerr << "BELReader: invalid format value " << fmt << "\n";
		return 0;
	}
	return 0;
}

int BELReader::loadFromStream( std::istream& fp )
{
	numStatements=numSkippedStatements=numProcs=numMacros=numEmits=0;

	if( !parser ) {
		std::cerr << "BELReader::loadFromStream uninitialized parser\n";
		return 0;
	}
	parser->parse( fp );
    // std::cerr << "Total Number of Emits: " << numEmits << std::endl;
	return numStatements;
}
void BELReader::computeImplicitTrieSpellPriority( uint32_t tc, uint32_t tid )
{
    const char* tclass = gp.internalString_resolve( tc );
	d_trieSpellPriority = ( (!tclass || !strlen(tclass)) ? 0 : 10 );
	d_spellPriority = d_rulesetSpellPriority+d_trieSpellPriority;
}

void BELReader::computeRulesetSpellPriority( const char* fileName )
{
	d_rulesetSpellPriority = 0;
	if( fileName ) {
		if( !strstr( fileName, "_fluff" ) )
			d_rulesetSpellPriority= 5;
	}
	d_spellPriority = d_rulesetSpellPriority+d_trieSpellPriority;
}

int BELReader::loadFromFile( const char* fileName, BELReader::InputFormat fmt )
{
	if( fileName )
		inputFileName.assign( fileName );

	if( !initParser(fmt) )
		return 0;

	if( inputFileName.length() ) { // trying to load from file
		std::ifstream fp;
		fp.open( inputFileName.c_str() );
		if( fp.is_open() ) {
			computeRulesetSpellPriority( fileName );
			return loadFromStream( fp );
		}else {
            ay::print_absolute_file_path( (std::cerr << "ERROR: BELReader cant open file \"" ), fileName ) << "\"\n";
			return -1;
		}
	} else {
		return loadFromStream( std::cin );
	}

	// should never get here but not all compilers warn about no return
	return 0;
}

size_t BELStatementParsed::setStmtNumber( size_t num ) 
{
    if( num )
        d_stmtNumber = num;
    else
        ++d_stmtNumber;
    return d_stmtNumber;
}
void BELStatementParsed::clear()
{
    d_tranUnmatchable= 0;
    d_confidenceBoost=0;
    d_execMode=EXEC_MODE_REWRITE;
    pattern.clear(); 
    translation.clear(); 
    d_ruleClashOverride=CLASH_OVERRIDE_NONE;
}
void BELStatementParsed::setRuleClashOverride( const char* str )
{
    if( !str ) {
        d_ruleClashOverride= CLASH_OVERRIDE_NONE;
        return;
    }
    switch( *str ) {
    case 'a':
        d_ruleClashOverride= CLASH_OVERRIDE_APPEND;
        break;
    case 'y':
    case 'Y':
    default:
        d_ruleClashOverride= CLASH_OVERRIDE_TAKELAST;
        break;
    }
}

BELStatementParsed::BELStatementParsed() :
		d_execMode(EXEC_MODE_REWRITE), 
        d_stmtNumber(0), 
        d_sourceNameStrId(0xffffffff), 
        d_ruleClashOverride(CLASH_OVERRIDE_NONE), 
        d_reader(0), 
        d_tranUnmatchable(0),
        d_confidenceBoost(0)
{ }


    BarzXMLErrorStream::BarzXMLErrorStream( std::ostream& o, const char* extraAttr ) : os(o) 
        { 
            os.os << "<error"; 
            if( extraAttr ) 
                os.os << " " << extraAttr;
            os.os << ">";
        }
    BarzXMLErrorStream::BarzXMLErrorStream( std::ostream& o, size_t stmtNum, const char* extraAttr ) : os(o) 
        { 
            os.os << "<error stmt=\""<< stmtNum << "\"" ;
            if( extraAttr ) 
                os.os << " " << extraAttr;
            os.os << ">"; 
        }
    BarzXMLErrorStream::BarzXMLErrorStream( BELReader& reader, size_t stmtNum, const char* extraAttr ) : 
        os(reader.getErrStreamRef())
    {
        os.os << "<error file=\"" << reader.getInputFileName() << "\" class=\"" << reader.getTrieClassName() << "\" trie=\"" << reader.getTrieName() << "\""
        << " stmt=\""<< stmtNum << "\"";
            if( extraAttr ) 
                os.os << " " << extraAttr;
        os.os << ">"; 
    }
    BarzXMLErrorStream::~BarzXMLErrorStream()
        { os.os << "</error>\n"; }
} // namespace barzer
