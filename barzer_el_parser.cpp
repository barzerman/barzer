#include <barzer_el_parser.h>
#include <barzer_el_xml.h>
#include <barzer_universe.h>
#include <barzer_spell.h>
#include <fstream>

#include <ay/ay_logger.h>
#include <ay_util_time.h>

namespace barzer {


const BELParseTreeNode* BELParser::getMacroByName( const std::string& macroname ) const
{
	const BELTrie& trie = reader->getTrie();
	return trie.getMacros().getMacro( macroname );
}

uint32_t BELParser::stemAndInternTmpText( const char* s, int len )
{
	std::string scopy(s, len );
	
	BarzerHunspellInvoke spellChecker(getGlobalPools().produceUniverse(0).getHunspell(), getGlobalPools());
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
	//uint32_t cwid = reader->getUniverse().getCompWordPool().addNewCompWordWithAlias( aliasId );
	GlobalPools &gp = reader->getGlobalPools();
	uint32_t cwid = gp.getCompWordPool().addNewCompWordWithAlias( aliasId );
	//StoredToken& sTok =  reader->getUniverse().getDtaIdx().addCompoundedToken(cwid);
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

	//StoredToken& sTok =  reader->getUniverse().getDtaIdx().addToken( t );
	StoredToken& sTok =  reader->getGlobalPools().getDtaIdx().addToken( t );
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


BELReader::BELReader( GlobalPools &g ) :
	trie(&g.globalTriePool.produceTrie("", "")) , parser(0), gp(g),
	numStatements(0) ,silentMode(false),  inputFmt(INPUT_FMT_XML)
{}

void BELReader::setTrie( const std::string& trieClass, const std::string& trieId )
{
	//trie = &(universe.produceTrie( trieClass, trieId ));
	trie = &(gp.globalTriePool.produceTrie( trieClass, trieId ));
}
std::ostream& BELReader::printNode( std::ostream& fp, const BarzelTrieNode& node ) const 
{
	BELPrintFormat fmt;
	//BELPrintContext ctxt( *trie, universe.getStringPool(), fmt );
	BELPrintContext ctxt( *trie, gp.getStringPool(), fmt );
	return node.print( fp, ctxt );
}
void BELReader::addMacro( const std::string& macroName, const BELStatementParsed& sp )
{
	BELParseTreeNode& storedMacro = getTrie().getMacros().addMacro( macroName ) ;
	storedMacro = sp.pattern;
}

void BELReader::addStatement( const BELStatementParsed& sp )
{
	BELParseTreeNode_PatternEmitter emitter( sp.pattern );
	//AYLOG(DEBUG) << "addStatement called";

	int i = 0, j = 0;
	ay::stopwatch totalTimer;
	//const StoredUniverse& universe = getUniverse();
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
	numStatements=0;
	if( !parser ) {
		std::cerr << "BELReader::loadFromStream uninitialized parser\n";
		return 0;
	}
	parser->parse( fp );

	return numStatements;
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
