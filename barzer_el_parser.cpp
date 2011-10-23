#include <barzer_el_parser.h>
#include <barzer_el_xml.h>
#include <barzer_universe.h>
#include <barzer_spell.h>
#include <fstream>

#include <ay/ay_logger.h>
#include <ay_util_time.h>

namespace barzer {


const BarzelEvalNode* BELParser::getProcByName( uint32_t strId ) const
{
	const BELTrie& trie = reader->getTrie();
	return trie.getProcs().getEvalNode( strId );
}
const BELParseTreeNode* BELParser::getMacroByName( const std::string& macroname ) const
{
	const BELTrie& trie = reader->getTrie();
	return trie.getMacros().getMacro( macroname );
}

uint32_t BELParser::stemAndInternTmpText( const char* s, int len )
{
	StoredUniverse* curUni = reader->getCurrentUniverse();
	if( !curUni ) 
		curUni = &(getGlobalPools().produceUniverse(0));
	BZSpell* bzSpell = curUni->getBZSpell();
	std::string scopy(s, len );
	if( bzSpell )  {
		std::string stem;
		if( bzSpell->stem(stem, scopy.c_str()) ) 
			internString( stem.c_str() );
	}
	return internString( scopy.c_str());
}
uint32_t BELParser::stemAndInternTmpText_hunspell( const char* s, int len )
{
	std::string scopy(s, len );
	StoredUniverse* curUni = reader->getCurrentUniverse();
	if( !curUni ) 
		curUni = &(getGlobalPools().produceUniverse(0));
	BarzerHunspellInvoke spellChecker(curUni->getHunspell(), getGlobalPools());
	const char* stemmed = spellChecker.stem(scopy.c_str());

	if( stemmed && strncmp(scopy.c_str(), stemmed, len) ) 
		internString( stemmed );

	return internString( scopy.c_str());
}

uint32_t BELParser::internVariable( const char* t )
{
	const char* beg = t;
	const char* end = strchr( t, '.' ); 
	std::string tmp;
	BELSingleVarPath vPath;
	for( ;end; ) {
		tmp.assign( beg, end-beg );
		vPath.push_back( internString(tmp.c_str()));
		beg = end+1;
		end = strchr( beg, '.' );
	}
	if( *beg ) {
		tmp.assign( beg );
		vPath.push_back( internString(tmp.c_str()));
	}
	return reader->getTrie().getVarIndex().produceVarIdFromPathForTran(vPath);
}

uint32_t BELParser::addCompoundedWordLiteral( const char* alias )
{
	uint32_t aliasId = ( alias ? internString(alias) : 0xffffffff );
	GlobalPools &gp = reader->getGlobalPools();
	uint32_t cwid = gp.getCompWordPool().addNewCompWordWithAlias( aliasId );
	StoredToken& sTok =  gp.getDtaIdx().addCompoundedToken(cwid);
	return sTok.tokId;
}

uint32_t BELParser::internString_internal( const char* t )
{
	return reader->getGlobalPools().internString_internal( t );
}
uint32_t BELParser::internString( const char* t )
{
	// here we may want to tweak some (nonexistent yet) fields in StoredToken 
	// to reflect the fact that this thing is actually in the trie

	StoredToken& sTok =  reader->getGlobalPools().getDtaIdx().addToken( t );
	BELTrie& trie = reader->getTrie();
	trie.addWordInfo( sTok.getStringId() );
    StoredUniverse* curUni = reader->getCurrentUniverse();
    if( curUni ) {
        BZSpell* bzSpell= curUni->getBZSpell();
        if( bzSpell ) {
            bzSpell->addExtraWordToDictionary( sTok.getStringId() );
        }
    }



	return sTok.getStringId();
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
    numMacros(0) , 
    numProcs(0) , 
    inputFmt(INPUT_FMT_XML),
    d_currentUniverse(0),
    d_errStream(errStream? errStream: &(std::cerr))
{}
BELReader::BELReader( GlobalPools &g, std::ostream* errStream ) : 
	trie(g.globalTriePool.produceTrie("", "")) , parser(0), gp(g),
	numStatements(0) ,silentMode(false),  
	d_trieSpellPriority(0),
	inputFmt(INPUT_FMT_XML),
	d_trieIdSet(false),
    d_errStream(errStream? errStream: &(std::cerr))
{}

void BELReader::setTrie( const std::string& trieClass, const std::string& trieId )
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
	int rc = getTrie().getProcs().generateStoredProc( strId, sp.translation );
	if( rc == BarzelProcs::ERR_OK ) {
		++numProcs;
	} else {
		AYLOG(ERROR) << "error=" << rc << ": failed to load procedure in statement " << numStatements << std::endl;
	}
	++numStatements;
}
void BELReader::addMacro( const std::string& macroName, const BELStatementParsed& sp )
{
	BELParseTreeNode& storedMacro = getTrie().getMacros().addMacro( macroName ) ;
	storedMacro = sp.pattern;
	++numMacros;
	++numStatements;
}

void BELReader::addStatement( const BELStatementParsed& sp )
{
	BELParseTreeNode_PatternEmitter emitter( sp.pattern );
	//AYLOG(DEBUG) << "addStatement called";

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
				tran->set(*trie, sp.translation);
                if( sp.isTranUnmatchable() ) 
                    tran->makeUnmatchable = 1;
			}	
			trie->addPath( sp, seq, tranId, varInfo, i );
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
			delete parser; 
			parser = 0;
		} else 
			return parser;
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
	numStatements=numProcs=numMacros=0;

	if( !parser ) {
		std::cerr << "BELReader::loadFromStream uninitialized parser\n";
		return 0;
	}
	parser->parse( fp );

	return numStatements;
}
void BELReader::computeImplicitTrieSpellPriority( const std::string& tclass, const std::string& trieId )
{
	d_trieSpellPriority = ( !tclass.length() ? 0 : 10 );
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
			std::cerr << "BELReader couldn't open file \"" << fileName << "\"\n";
			return 0;
		}
	} else {
		return loadFromStream( std::cin );
	}
	
	// should never get here but not all compilers warn about no return 
	return 0;
}


} // namespace barzer 
