#include <barzer_el_parser.h>
#include <barzer_el_xml.h>
#include <fstream>

#include "ay/ay_logger.h"

namespace barzer {

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

void BELReader::addStatement( const BELStatementParsed& sp )
{
	BELParseTreeNode_PatternEmitter emitter( sp.pattern );


	int i = 0;
	while( emitter.produceSequence() ) {
		const BTND_PatternDataVec& seq = emitter.getCurSequence();
		for (BTND_PatternDataVec::const_iterator ci = seq.begin(); ci != seq.end(); ++ci) {
			const BTND_PatternData &pd = *ci;
			printPatternData(std::cout, pd);
		}
		trie->addPath( seq, sp.translation );
		i++;
	}
	AYLOG(DEBUG) << i << " sequences produced";

	
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
		return ( (parser = new BELParserXML( this, strPool )) );
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
