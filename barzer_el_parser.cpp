#include <barzer_el_parser.h>
#include <barzer_el_xml.h>
#include <barzer_universe.h>
#include <fstream>

#include <ay/ay_logger.h>
#include <ay_util_time.h>

namespace barzer {

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

uint32_t BELParser::internString( const char* t )
{
	// here we may want to tweak some (nonexistent yet) fields in StoredToken 
	// to reflect the fact that this thing is actually in the trie
	StoredToken& sTok =  reader->getUniverse().getDtaIdx().addToken( t );
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

std::ostream& BELReader::printNode( std::ostream& fp, const BarzelTrieNode& node ) const 
{
	BELPrintFormat fmt;
	BELPrintContext ctxt( *trie, universe.getStringPool(), fmt );
	return node.print( fp, ctxt );
}
void BELReader::addStatement( const BELStatementParsed& sp )
{
	BELParseTreeNode_PatternEmitter emitter( sp.pattern );
	//AYLOG(DEBUG) << "addStatement called";

	int i = 0, j = 0;
	ay::stopwatch totalTimer;
	do {
		const BTND_PatternDataVec& seq = emitter.getCurSequence();
		const BELVarInfo& varInfo = emitter.getVarInfo();
		j += seq.size();
		uint32_t tranId = 0xffffffff;
		BarzelTranslation* tran = trie->makeNewBarzelTranslation( tranId );

		if( !tran ) {
			AYLOG(ERROR) << "null translation returned\n";
		} else
			tran->set(*trie, sp.translation);
		trie->addPath( seq, tranId, varInfo );
		i++;
		//AYLOG(DEBUG) << "path added";
	} while( emitter.produceSequence() );
	// AYLOG(DEBUG) << i << " sequences(" <<  j << " nodes) produced in " << totalTimer.calcTime();
	
	++numStatements;
	// AYDEBUG(numStatements);
}


BELParser*  BELReader::initParser(InputFormat fmt)
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
	case INPUT_FMT_XML:
		return ( (parser = new BELParserXML( this )) );
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

	if( !initParser( fmt ) )
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
