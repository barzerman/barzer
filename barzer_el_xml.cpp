#include <barzer_emitter.h>
#include <barzer_el_xml.h>
#include <barzer_universe.h>
#include <barzer_ghettodb.h>
#include <ay/ay_debug.h>
#include <barzer_server_response.h>
#include <ay_xml_util.h>
#include <ay_translit_ru.h>
#include <barzer_relbits.h>
#include "barzer_spellheuristics.h"
#include "barzer_geoindex.h"
extern "C" {
#include <expat.h>

// cast to XML_StartElementHandler

static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n;
	const char** atts = (const char**)a; 
	barzer::BELParserXML *loader =(barzer::BELParserXML *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( loader->parser );
	if( attrCount <=0 || (attrCount & 1) )
		attrCount = 0; // odd number of attributes is invalid
	loader->startElement( name, atts, attrCount );
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	barzer::BELParserXML *loader =(barzer::BELParserXML *)ud;
	loader->endElement(name);
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
    if( !len )
        return;
	const char* s = (const char*)str;
	barzer::BELParserXML *loader =(barzer::BELParserXML *)ud;
	if( len>1 || !isspace(*s) ) 
		loader->getElementText( s, len );
}
} // extern "C" ends


namespace barzer {

namespace {

void escapeForXmlAndIntern( GlobalPools& gp, uint32_t& srcNameStrId, std::string& s, const char* cs ) 
{
    std::stringstream sstr; 
    xmlEscape( cs, sstr );
    s=sstr.str();
    srcNameStrId = gp.internString_internal( s.c_str() );
}

}

BELParserXML::BELParserXML( BELReader* r) : 
		BELParser(r),
		parser(0),
		statementCount(0)
	{
		const char* tmp_srcName =reader->getInputFileName().c_str();

		uint32_t srcNameStrId ;
        std::string srcName; 
        {
        std::stringstream sstr;
        xmlEscape( tmp_srcName, sstr );
        srcName = sstr.str();
		srcNameStrId = reader->getGlobalPools().internString_internal( srcName.c_str() );
        }

		statement.stmt.setSrcInfo(srcName.c_str(), srcNameStrId );
        statement.stmt.setReader(r);
	}
bool BELParserXML::isValidTag( int tag, int parent ) const
{
	switch( parent ) {
	case TAG_UNDEFINED:
		// in fact statement can't even have an undefined parent anymore.
		// should probably rewrite it /pltr
		return ( tag == TAG_STATEMENT || tag == TAG_STMSET);
	case TAG_STATEMENT:
		if( tag == TAG_PATTERN ) {
			if( !statement.hasPattern() )
				return true;
		} else if( tag == TAG_TRANSLATION ) {
			if( !statement.hasTranslation() )
				return true;
		}
		return false;
	case TAG_PATTERN:
	switch( tag ) {
    /// pattern eligible child tags 
	case TAG_T:
	case TAG_VAR:
	case TAG_TG:
	case TAG_P:
	case TAG_SPC:
	case TAG_N:
	case TAG_RX:
	case TAG_TDRV:
	case TAG_WCLS:
	case TAG_W:
	case TAG_DATE:
	case TAG_DATETIME:
	case TAG_ENTITY:
	case TAG_RANGE:
	case TAG_ERCEXPR:
	case TAG_ERC:
	case TAG_EXPAND:
	case TAG_TIME:
	case TAG_LIST:
	case TAG_ANY:
	case TAG_OPT:
	case TAG_PERM:
	case TAG_TAIL:
	case TAG_SUBSET:
		return true;
	default:
		return false;
	}
		break;
	case TAG_TRANSLATION:
		return true;
    case TAG_STMSET: /// MKENT and STATEMENT can be children of STMSET
        return ( tag == TAG_MKENT || tag == TAG_STATEMENT );
    case TAG_MKENT:
        return ( tag == TAG_NV );
	default:
		return true;
	}
	// fall through - should reach here 	
	return false;
}

// attr_sz number of attribute pairs
void BELParserXML::startElement( const char* tag, const char_cp * attr, size_t attr_sz )
{
    cdataBuf.clear();
	int tid = getTag( tag );
	if( tid == TAG_UNDEFINED && statement.hasStatement() ) 
		statement.setInvalid();

	int parentTag = ( tagStack.empty() ? TAG_UNDEFINED:  tagStack.top() );
	
	if( tid == TAG_STATEMENT ) {
		++statementCount;
	}
	tagStack.push( tid );
	if( !isValidTag( tid, parentTag )  ) {
        {
        BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
		errStream.os << "invalid BEL xml(tag: " << tag <<")";
        }
		tagStack.pop();
		return;
	}	
	elementHandleRouter( tid, attr,attr_sz, false );
}

void BELParserXML::elementHandleRouter( int tid, const char_cp * attr, size_t attr_sz, bool close )
{
    if( statement.isDisabled() ) {
        if( close ) {
            if( tid == TAG_STATEMENT ) 
                statement.setDisabled(false);
        }
        return;
    }
	//AYDEBUG(tid);
#define CASE_TAG(x) case TAG_##x: taghandle_##x(tid,attr,attr_sz,close); return;

	switch( tid ) {
	CASE_TAG(T)
	CASE_TAG(N)
	CASE_TAG(MKENT)
	CASE_TAG(VAR)
	CASE_TAG(FUNC)
	CASE_TAG(LIST)
	CASE_TAG(ANY)
	CASE_TAG(OPT)
	CASE_TAG(STATEMENT)
	CASE_TAG(GET)
	CASE_TAG(SET)
	CASE_TAG(STMSET)
	CASE_TAG(PATTERN)
	CASE_TAG(TRANSLATION)
	CASE_TAG(TG)
	CASE_TAG(UNDEFINED)
	CASE_TAG(P)
	CASE_TAG(SPC)
	CASE_TAG(RX)
	CASE_TAG(TDRV)
	CASE_TAG(WCLS)
	CASE_TAG(W)
	CASE_TAG(DATE)
	CASE_TAG(DATETIME)
	CASE_TAG(ENTITY)
	CASE_TAG(RANGE)
	CASE_TAG(ERCEXPR)
	CASE_TAG(EXPAND)
	CASE_TAG(ERC)
	CASE_TAG(TIME)
	CASE_TAG(PERM)
	CASE_TAG(TAIL)
	CASE_TAG(SUBSET)

	CASE_TAG(LITERAL)
	CASE_TAG(RNUMBER)
	CASE_TAG(NV)
	CASE_TAG(SELECT)
	CASE_TAG(CASE)
	CASE_TAG(BLOCK)

	// special cases
	case TAG_AND:
	case TAG_OR:
	case TAG_NOT:
	    processLogic(tid, close);
	    return;
	case TAG_TEST:
	    processLogic(TAG_OR, close);
	    return;
	case TAG_COND:
	    processLogic(TAG_AND, close);
	    return;
	}
#undef CASE_TAG

	//if( close )
	//	statement.popNode();

}

#define DEFINE_BELParserXML_taghandle( X )  void BELParserXML::taghandle_##X(int tid, const char_cp * attr, size_t attr_sz, bool close )
DEFINE_BELParserXML_taghandle(STMSET)
{
	uint32_t trieClass =0xffffffff, trieId=0xffffffff;
    const char* tc =0, *ti =0;
	if( close ) {
        trieClass = reader->getGlobalPools().internString_internal("") ;
        trieId = reader->getGlobalPools().internString_internal("") ;
		reader->setTrie(trieClass,trieId);
		return;
	}
    reader->setDefaultExecMode( BELStatementParsed::EXEC_MODE_REWRITE );
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'c': 
            trieClass = reader->getGlobalPools().internString_internal(v) ;
            tc = v;
            break;
		case 'i':
            trieId = reader->getGlobalPools().internString_internal(v) ;
            ti =v;
            break;
        case 'm': // execute mode (imperative is one option)
            if( v[0] == 'p' ) // imperative PRE
                reader->setDefaultExecMode( BELStatementParsed::EXEC_MODE_IMPERATIVE_PRE );
            else if( v[0] =='s' ) 
                reader->setDefaultExecMode( BELStatementParsed::EXEC_MODE_IMPERATIVE_POST );
            break;
        case 't':
            reader->setTagFilter(v);
            break;
		}
	}

    if( !tc )
        trieClass = reader->getGlobalPools().internString_internal("") ;
    if( !ti )
        trieId = reader->getGlobalPools().internString_internal("") ;
        
	if ( tc || ti ) 
		reader->setTrie( trieClass , trieId );
    // setting current universe  
}
DEFINE_BELParserXML_taghandle(STATEMENT)
{
	if( close ) { /// statement is ready to be sent to the reader for adding to the trie
		if( !statement.isValid() ) {
            BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
			errStream.os << "skipped invalid statement ";
		} else {
			if( statement.isMacro() ) {
				reader->addMacro( statement.macroNameId, statement.stmt );
			} else if( statement.isProc() ) {
				reader->addProc( statement.procNameId, statement.stmt );
			} else if( statement.hasStatement() && statement.hasPattern() ) {
				reader->addStatement( statement.stmt );
            }
		}
		statement.clear();
		return;
	}
	size_t stmtNumber = 0;
    bool needAbort = false;
    std::stringstream errStrStr ;
    bool tagsPassFilter = false;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'm': 
			if( !getMacroByName( v )) {
                uint32_t macroStrId  = reader->getGlobalPools().internString_internal(v) ;
				statement.setMacro(macroStrId); // m="MACROXXX"
			} else {
				errStrStr << "attempt to REDEFINE MACRO " << v  << " ignored";
                needAbort  = true;
			}
            break;
		case 'p':  //  proc
			{ // block
			uint32_t procNameStrId = reader->getGlobalPools().internString_internal( v );

			if( !getProcByName(procNameStrId)) {
				statement.setProc(procNameStrId); // p="PROCXXX"
			} else {
				errStrStr << "attempt to REDEFINE Procedure " << v  << " ignored";
				needAbort  = true;
			}
			}  // end of block
            break;
		case 'n':  // statement number
            if( !n[1] )
			    stmtNumber = atoi(v);
            // name - is for barsted names 
			break;
		case 'd':
			if( v && (v[0] == 'y'|| v[0]=='Y') ) {
				statement.setDisabled();
				return;
			}
			break;
        case 'o':
            if( v && v[0] != 'n'&&v[0]!='N' ) 
                statement.stmt.setRuleClashOverride();
            break;
		case 'x':  // pipe separated tags
            if( reader->hasTagFilters() ) {
                if( !reader->tagsPassFilter( v ) ) {
                    statement.setInvalid();
                    return;
                } else
                    tagsPassFilter = true;
            }
			break;
		default:
            {
			BarzXMLErrorStream errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
            errStream.os << "unknown statement attribute " << n << ": " << v;
            }
			break;
		}
	}
    if( !tagsPassFilter && reader->hasTagFilters() ) {
        statement.setInvalid();
        return;
    }
        
    if( stmtNumber == 145705 ) {
        std::cerr << 145705 << " reached\n";
    }
    if( needAbort ) {
	    statement.stmt.setStmtNumber( stmtNumber ) ;
        BarzXMLErrorStream  errStream(reader->getErrStreamRef(),stmtNumber);
        errStream.os << errStrStr.str();
        return;
    }

	// const BELParseTreeNode* macroNode = getMacroByName(macroName);

	statement.stmt.setStmtNumber( stmtNumber ) ;
	if( !reader->isSilentMode() && !(statement.stmt.getStmtNumber() % 512)  ) {
		std::cerr << '.';
	}
	if( statement.isMacro() ) {
	} else {
		if( statement.hasStatement() ) { // bad - means we have statement tag nested in another statement
            BarzXMLErrorStream  errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
            errStream.os << "statement nested in statement";
			return;
		} 
	}
	statement.setStatement();
}
DEFINE_BELParserXML_taghandle(UNDEFINED)
{}

DEFINE_BELParserXML_taghandle(PATTERN)
{
	if( close ) { /// tag is being closed
		statement.setBlank();
		return;
	}
	if( statement.hasPattern() ) { 
		BarzXMLErrorStream  errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
        errStream.os << "duplicate pattern tag";
		return;
	}
	statement.setPattern();
}
DEFINE_BELParserXML_taghandle(TRANSLATION)
{
	if( close ) { /// tag is being closed
		statement.setBlank();
		return;
	}
	if( statement.hasTranslation() ) { 
        BarzXMLErrorStream  errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
		errStream.os << "duplicate translation tag in statement";
		return;
	}
	statement.setTranslation();
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
        if(!n) continue;
        switch( n[0] ) {
        case 'u': if( v && v[0] == 'y' ) statement.stmt.setTranUnmatchable(); break;
        }
    }
}
/// tag text visitors
namespace {

struct BTND_text_visitor_base  : public boost::static_visitor<> {
	BELParserXML& d_parser;
	const char* d_str;
	size_t d_len;
    uint8_t d_noTextToNum;

	GlobalPools& getGlobalPoools() { return d_parser.getGlobalPools(); }
	const GlobalPools& getGlobalPoools() const { return d_parser.getGlobalPools(); }

	bool isAnalyticalMode() const
	//{ return getUniverse().isAnalyticalMode(); }
		{ return getGlobalPoools().isAnalyticalMode(); }

	BTND_text_visitor_base( BELParserXML& parser, const char* s, int len, uint8_t noTextToNum ) : 
		d_parser(parser),
		d_str(s), 
		d_len(len) ,
        d_noTextToNum(noTextToNum)
	{}
};

// rewrite visitor template specifications 
struct BTND_Rewrite_Text_visitor : public BTND_text_visitor_base {
	BTND_Rewrite_Text_visitor( BELParserXML& parser, const char* s, int len, uint8_t noTextToNum ) :
		BTND_text_visitor_base( parser, s, len, noTextToNum ) 
	{}

	template <typename T>
	void operator()(T& t) const { }
};
template <> void BTND_Rewrite_Text_visitor::operator()<BTND_Rewrite_Literal>(BTND_Rewrite_Literal& t)   const
	{ 
        const char* theStr = ( d_str[d_len] ? d_parser.setTmpText(d_str,d_len) : d_str );
        if( t.isInternalString() ) {
            t.setId( d_parser.getGlobalPools().internString_internal( theStr, d_len ) );
        } else {
		    uint32_t i = d_parser.getGlobalPools().string_intern( theStr, d_len );
		    if( t.isCompound() ) {
			    // warning compounded tokens unsupported yet
		    } else {
			    t.setId( i );
		    }
            // all right side literals will be internally interned as well
            d_parser.getGlobalPools().internString_internal( theStr, d_len );
        }
	}
template <> void BTND_Rewrite_Text_visitor::operator()<BTND_Rewrite_Number>(BTND_Rewrite_Number& t)   const
	{ 
		const char* s = d_parser.setTmpText( d_str, d_len );
		if( strchr( s, '.' ) ) 
			t.set( atof(s) );
		else
			t.set( atoi(s) );
	}

// pattern visitor  template specifications 
struct BTND_Pattern_Text_visitor : public BTND_text_visitor_base {
	BTND_PatternData& d_pat;
    bool d_isNotNew; 
	BTND_Pattern_Text_visitor( BTND_PatternData& pat, BELParserXML& parser, const char* s, int len, uint8_t noTextToNum ) :
		BTND_text_visitor_base( parser, s, len, noTextToNum ),
		d_pat(pat),
        d_isNotNew(false)
	{}
    bool tokenWasNew() const { return !d_isNotNew; }

	template <typename T>
	void operator() (T& t) {}
};

namespace {
bool isAllDigits( const char* s,int len  ) {
	const char* s_end = s+len;
	for( ; s< s_end; ++s ) {
		if( !isdigit(*s) ) 
			return false;
	}
	return true;
}
} // anon namespace ends 

template <> void BTND_Pattern_Text_visitor::operator()<BTND_Pattern_Token>  (BTND_Pattern_Token& t)
{ 
	/// if d_str is numeric we need to do something
    const char* str = ( d_str[d_len] ? (d_parser.setTmpText(d_str,d_len)) : d_str );

    if( const StoredUniverse* uni = d_parser.getReader()->getCurrentUniverse() ) {
        if( const StoredToken* storedTok = uni->getStoredToken(str) ) {
            if( const BZSpell* bzSpell = uni->getBZSpell() ) {
                d_isNotNew =  bzSpell->isUsersWordById(storedTok->getStringId());
            }
        }
    }

    bool isNumeric = (isdigit(str[0]) && isAllDigits(str,d_len));
	bool strIsNum = ( !d_noTextToNum && isNumeric );
	bool needStem = t.doStem;
	if( strIsNum ) {
		if( !isAnalyticalMode() ) {
			BTND_Pattern_Number numPat;
			int num= atoi(str);
			numPat.setIntRange( num, num );
			d_pat = numPat;
			return;
		}
	}  else {
		///
		if( needStem && (!d_parser.getGlobalPools().parseSettings().stemByDefault() || isNumeric) ) 
			needStem = false;
	}
	if( needStem ) 
		t.stringId = d_parser.stemAndInternTmpText(str, d_len); 
	else
		t.stringId = d_parser.internTmpText(  str, d_len, false, isNumeric); 
}
template <> void BTND_Pattern_Text_visitor::operator()<BTND_Pattern_StopToken>  (BTND_Pattern_StopToken& t)
{ 
	t.stringId = d_parser.internTmpText(  d_str, d_len, false, false /*isNumeric*/ ); 
}
template <> void BTND_Pattern_Text_visitor::operator()<BTND_Pattern_Punct> (BTND_Pattern_Punct& t)
{ 
	// may want to do something fancy for chinese punctuation (whatever that is)
	const char* str_end = d_str + d_len;
	for( const char* s = d_str; s< str_end; ++s ) {
		if( !isspace(*s) ) {
			t.setChar( *s );
			return;
		}
	}
	t.setChar(0);
}
// pattern visitor  template specifications 

uint32_t tryGetPrioMeaning(const StoredUniverse& uni, uint32_t wordId)
{
    const MeaningsStorage& meanings = uni.meanings();
	const uint8_t threshold = meanings.getAutoexpThreshold();

	switch (meanings.getAutoexpMode()) {
	case MeaningsStorage::MeaningsAutoexp::None:
		return 0xffffffff;
	case MeaningsStorage::MeaningsAutoexp::One: {
		const WordMeaningBufPtr& buf = meanings.getMeanings(wordId);
		if (buf.second == 1 && buf.first->prio <= threshold)
			return buf.first->id;
		else
			return 0xffffffff;
    }
        break;
	case MeaningsStorage::MeaningsAutoexp::Dominant: {
		uint8_t     max = 0xff, preMax = 0xff;
		uint32_t    maxId = 0xffffffff;

		const WordMeaningBufPtr& buf = meanings.getMeanings(wordId);
		for (const WordMeaning *m = buf.first, *end = buf.first + buf.second; m < end; ++m) {
			if (m->prio < max) {
                maxId = m->id;
                preMax = max;
                max = m->prio;
            }
		}

		return (preMax >= (max+threshold) ?  maxId : 0xffffffff );
    }
        break;
	} // switch ends

	return 0xffffffff;
}

bool tryExpandMeaning(BTND_PatternData& pat, const StoredUniverse& uni)
{
	if (pat.which() != BTND_Pattern_Token_TYPE)
		return false;

	const BTND_Pattern_Token& tok = boost::get<BTND_Pattern_Token>(pat);
	const uint32_t meaningId = tryGetPrioMeaning(uni, tok.getStringId());
	if (meaningId == 0xffffffff)
		return false;

	BTND_Pattern_Meaning m;
	m.meaningId = meaningId;
	m.d_matchMode = tok.d_matchMode;
	pat = m;
	return true;
}

void addSLInfo (StoredUniverse *uni, const BTND_Pattern_Token& patTok)
{
	const uint32_t strId = patTok.getStringId();
	BZSpell *spell = uni->getBZSpell();
	if (!spell)
		return;
	
	GlobalPools& gp = uni->getGlobalPools();
	const char *str = gp.string_resolve(strId);
	if (!str)
		return;
	
	const size_t len = std::strlen(str);
	if (Lang::getLang(*uni, str, len) != LANG_ENGLISH)
		return;
	
	spell->getEnglishSL().addSource(str, len, strId);
}

// general visitor 
struct BTND_text_visitor : public BTND_text_visitor_base {

	BTND_text_visitor( BELParserXML& parser, const char* s, int len, uint8_t noTextToNum ) :
		BTND_text_visitor_base( parser, s, len,noTextToNum) 
	{}
	void operator()( BTND_PatternData& pat ) const
    { 
		BTND_Pattern_Text_visitor vis(pat, d_parser, d_str, d_len, d_noTextToNum);
		boost::apply_visitor( vis, pat ) ;
		
		const bool isToken = pat.which() == BTND_Pattern_Token_TYPE;

		StoredUniverse* uni = d_parser.getReader()->getCurrentUniverse();

		bool meaningExpanded = false;
		//// relelase bit - by defaults autoexpansion is on
        if( isToken && uni ) {
            tryExpandMeaning(pat, *uni);

            if(uni->soundsLikeEnabled() && vis.tokenWasNew() )
                addSLInfo(uni, boost::get<BTND_Pattern_Token>(pat));
        }
    }
	void operator()( BTND_None& ) const {}
	void operator()( BTND_StructData& ) const {}
	void operator()( BTND_RewriteData& rwr)  const
		{ boost::apply_visitor( BTND_Rewrite_Text_visitor(d_parser,d_str,d_len,0), rwr ) ; }
};
} // end of anon namespace 

DEFINE_BELParserXML_taghandle(T)
{
    if( statement.isState_Translation() ) {
        taghandle_LITERAL(TAG_T,attr, attr_sz, close);
        return;
    } 

	if( close ) {
		statement.popNode();
		return;
	}
	bool isStop = false, noTextToNum = true;
	bool doStem = getGlobalPools().parseSettings().stemByDefault() ;
    const char* modeString = 0;
	uint32_t meaningId = 0xffffffff;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'f':  // force number
            noTextToNum = false;
            break;
		case 't':  // type t=
			switch( v[0] ) {
			// stop is the same as fluff
			case 'f': // fluff (t="f" or t="fluff")
			case 's': // stop (t="s" or t="stop") 
				isStop = true;
				break;
			}
			break;
		case 's':  // s="n" - no stemming
			if( getGlobalPools().parseSettings().stemByDefault() )
				doStem = ( *v != 'n' );
			break;
        case 'x': 
            modeString = v;
            break;
		case 'm':
			meaningId = getGlobalPools().internalString_getId(v);
			if (meaningId == 0xffffffff)
				AYLOG(ERROR) << "unknown meaning " << v;
			break;
		}
	}

	BTND_PatternData dta;
	if( isStop ) {
		BTND_Pattern_StopToken p;
        if( modeString ) 
            p.setMatchModeFromAttribute(modeString);
		dta = p;
	}
	else if (meaningId != 0xffffffff)
	{
		BTND_Pattern_Meaning m;
		m.meaningId = meaningId;
		if( modeString ) 
			m.setMatchModeFromAttribute(modeString);
		dta = m;
	}
	else
	{
		BTND_Pattern_Token p;
		p.doStem = doStem;
        
        if( modeString ) 
            p.setMatchModeFromAttribute(modeString);
		dta =p ;
	}
	BELParseTreeNode* newNode = statement.pushNode( dta );
    if( newNode && noTextToNum ) {
        newNode->noTextToNum = noTextToNum;
    }
}

DEFINE_BELParserXML_taghandle(TG)
{
}
DEFINE_BELParserXML_taghandle(P)
{
	if (close) {
		statement.popNode();
		return;
	} 
	statement.pushNode( BTND_PatternData( BTND_Pattern_Punct()));
}
DEFINE_BELParserXML_taghandle(SPC)
{
}
DEFINE_BELParserXML_taghandle(N)
{
    if( statement.isState_Translation() ) {
        taghandle_RNUMBER( TAG_N, attr, attr_sz, close );
        return;
    }

	if( close ) {
		statement.popNode();
		return;
	}
	bool isReal = false, isRange = false;
	float flo=0., fhi = 0.;
	int   ilo = 0, ihi = 0;
	BTND_Pattern_Number pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( *n ) {
		case 'h':
			if( !isRange ) isRange = true;

			if( !isReal ) 
				isReal = strchr(v, '.');
			if( isReal ) {
				fhi = atof( v );
				ihi = (int) fhi;
			} else {
				ihi = atoi( v );
				fhi = (float) ihi;
			}
			break;
		case 'l':
			if( !isRange ) isRange = true;
			if( !isReal ) 
				isReal = strchr(v, '.');
			if( isReal ) {
				flo = atof( v );
				ilo = (int) flo;
			} else {
				ilo = atoi( v );
				flo = (float) ilo;
			}
			break;
		case 'r':
			if( !isReal ) isReal = true;
			break;
		case 'w':{
			int width = atoi(v);
			pat.setAsciiLen( width );
		}
			break;
		}
	}
	
	if( isRange ) {
		if( isReal ) 
			pat.setRealRange( flo, fhi );
		else 
			pat.setIntRange( ilo, ihi );
	} else {
		if( isReal ) 
			pat.setAnyReal();
		else
			pat.setAnyInt();
	}
	statement.pushNode( BTND_PatternData( pat));
}
DEFINE_BELParserXML_taghandle(ERC)
{
	if( close ) { statement.popNode(); return; }
	BTND_Pattern_ERC pat; 
	BarzerEntityRangeCombo& erc = pat.getERC();

	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'b': // match blank range / ent
			switch( n[1] ) {
			case 'r': // br="y" - match blank range
				pat.setMatchBlankRange();
				break;
			case 'e': // be="y" - match blank entity id
				pat.setMatchBlankEntity();
				break;
			}
			break;
		case 'c': // entity class - c="1"
			erc.getEntity().setClass( atoi(v) ); 
			break;
		case 's': // entity subclass - s="1"
			erc.getEntity().setSubclass( atoi(v) ); 
			break;
		case 'i': // entity id token - t="ABCD011"
		case 't': // entity id token - t="ABCD011"
			// erc.getEntity().setTokenId( internString(v,true).getStringId() );
			erc.getEntity().setTokenId( reader->getGlobalPools().internString_internal(v) );
			break;
		case 'u': // unit fields
			switch(n[1]) {
			case 'c': // unit entity  class - uc="1"
				erc.getUnitEntity().setClass( atoi(v) ); break;
			case 's': // unit entity subclass - us="1"
				erc.getUnitEntity().setSubclass( atoi(v) ); break;
			case 'i': // unit entity id token - ut="ABCD011"
			case 't': // unit entity id token - ut="ABCD011"
				// erc.getUnitEntity().setTokenId( internString(v,true).getStringId() ); break;
				erc.getUnitEntity().setTokenId( reader->getGlobalPools().internString_internal(v) );
			}
			break;
		case 'r': // range type setting 
			switch( v[0] ) {
			case 'n':
				erc.getRange().setData( BarzerRange::None() );
				break;
			case 'i':  // r=i
				erc.getRange().setData( BarzerRange::Integer() );
				break;
			case 'r':  // r=r real numbers
				erc.getRange().setData( BarzerRange::Real() );
				break;
			case 't':  // r=t - time of day
				erc.getRange().setData( BarzerRange::TimeOfDay() );
				break;
			case 'd':  // r=d - date
				erc.getRange().setData( BarzerRange::Date() );
				break;
			case 'm':  // r=m - date time
				erc.getRange().setData( BarzerRange::DateTime() );
				break;
			case 'e':  // r=e - entity
				erc.getRange().setData( BarzerRange::Entity() );
				break;
			}
			break;
        case 'x': 
            pat.setMatchModeFromAttribute(v);
            break;
		}
	}

	statement.pushNode( BTND_PatternData(pat) );
}
DEFINE_BELParserXML_taghandle(ERCEXPR)
{
	if( close ) { statement.popNode(); return; }
	BTND_Pattern_ERCExpr pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'c': // c - eclass
			pat.setEclass( atoi(v) );
			break;
        case 'x': 
            pat.setMatchModeFromAttribute(v);
            break;
		}
	}
	statement.pushNode( BTND_PatternData(pat) );
}
DEFINE_BELParserXML_taghandle(RANGE)
{ 
	if( close ) { statement.popNode(); return; }
	BTND_Pattern_Range pat; 

	/// entity range related locals 
	StoredEntityUniqId euid;
	const char* id1Str = 0;
	const char* id2Str = 0;
	bool isEntity = false;
	/// end of entity range related stuff 
    bool isNumericFlavor= false;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value

		switch( n[0] ) {
		case 'm': // class - c="1"
			if( *v == 'v' ) 
				pat.setModeToVal( );
			break;
		case 't': // type 
			switch( v[0] ) {
			case 'i': pat.range().dta = BarzerRange::Integer(); break;
			case 'r': pat.range().dta = BarzerRange::Real(); break;
			case 't': pat.range().dta = BarzerRange::TimeOfDay(); break;
			case 'm': pat.range().dta = BarzerRange::DateTime(); break;
			case 'd': pat.range().dta = BarzerRange::Date(); break;
			case 'e': if( !isEntity ) isEntity= true; break;
			}
			break;
		case 'n':
            isNumericFlavor= true;
            break;
		case 'e':
			if( !isEntity ) isEntity= true;

			switch( n[1] ) {
			case 's': euid.eclass.subclass =  atoi(v); break; // "es" - subclass
			case 'c': euid.eclass.ec =  atoi(v); break; 	  // "ec" - class
			case 'i': 
				if( n[2] == '1' ) { 						  // i1=id - first entity id 
					id1Str = v;
				} else if( n[2] == '2' ) { 					  // i2=id - second entity id
					id2Str = v;
				}
			}
			break;
        case 'x': 
            pat.setMatchModeFromAttribute(v);
            break;
		}
	}
	if( isEntity ){
		pat.range().dta = BarzerRange::Entity();

		if( euid.eclass.isValid() ) {
			BarzerRange::Entity& entRange = pat.range().setEntityClass( euid.eclass ); 
	
			if( id1Str ) {
                uint32_t id1StrId = reader->getGlobalPools().internString_internal(id1Str) ;
				const StoredEntity& ent1  = 
					//reader->getUniverse().getDtaIdx().addGenericEntity( id1Str, euid.eclass.ec, euid.eclass.subclass );
					reader->getGlobalPools().getDtaIdx().addGenericEntity( id1StrId, euid.eclass.ec, euid.eclass.subclass );
				entRange.first = ent1.getEuid();
			}
			if( id2Str ) {

                uint32_t id2StrId = reader->getGlobalPools().internString_internal(id2Str) ;
				const StoredEntity& ent2  = 
					//reader->getUniverse().getDtaIdx().addGenericEntity( id2Str, euid.eclass.ec, euid.eclass.subclass );
					reader->getGlobalPools().getDtaIdx().addGenericEntity( id2StrId, euid.eclass.ec, euid.eclass.subclass );
				entRange.first = ent2.getEuid();
			}
		}
	} else if( pat.range().isNumeric() )  {
        if( isNumericFlavor ) 
            pat.setFlavorNumeric();
    }
	statement.pushNode( BTND_PatternData( pat));
}
DEFINE_BELParserXML_taghandle(ENTITY)
{ 
	if( close ) { statement.popNode(); return; }
	BTND_Pattern_Entity pat; 
    
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'e': // class - ec="1" (same as c)
            if( n[1] == 'c' ) 
			    pat.setEntityClass( atoi(v) ); 
            break;
		case 'c': // class - c="1"
			pat.setEntityClass( atoi(v) ); 
			break;
		case 's': // subclass - s="1"
			pat.setEntitySubclass( atoi(v) ); 
			break;
		case 'i': // 
		case 't': // id string - t="ABCD011"
            {
            uint32_t idStrId = reader->getGlobalPools().internString_internal(v) ;
			pat.setTokenId(idStrId);
            }
			// pat.setTokenId( internString(v,true).getStringId() );
			break;
        case 'x': // match mode 
            pat.setMatchModeFromAttribute(v);    
            break;
		}
	}
	statement.pushNode( BTND_PatternData( pat));
}
DEFINE_BELParserXML_taghandle(DATETIME)
{ 
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Pattern_DateTime pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'd': 
			if( n[1] == 'l' ) { /// dl="YYYYMMDD" date low 
				pat.setLoDate( atoi(v) );
			} else if( n[1] == 'h' ) { // dh="YYYYMMDD" date hi
				pat.setHiDate( atoi(v) );
			}
			break;
		case 'f':  // f="y" - future
			pat.setFuture();
			break;
		case 'p':  // f="y" - future
			pat.setPast();
			break;
		case 't': 
			if( n[1] == 'l' ) { /// tl="hhmmss" time low 
				pat.setLoTime( atoi(v) );
			} else if( n[1] == 'h' ) { // th="hhmmss" time hi
				pat.setHiTime( atoi(v) );
			}
			break;
        case 'x': 
            pat.setMatchModeFromAttribute(v);
            break;
		}
	}
	statement.pushNode( BTND_PatternData( pat));
}
DEFINE_BELParserXML_taghandle(DATE)
{ 
	if( close ) {
		statement.popNode();
		return;
	}
	/// we can actually replace future and past with ranges, where today is computed for the
	/// current instance of the engine in case there are problems 
	BTND_Pattern_Date pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		/// note: value of the attribute is ignored for everything except for hi and lo 
		/// all other attributes are boolean
		switch( *n ) {
		// any <date a="y"/> - thi is a default option <date/> accomplishes just that
		case 'a': pat.setPast(); break;
		// future <date f="y"/> 
		case 'f': pat.setFuture(); break;
		// high same as low 'l'
		case 'h': pat.setLo(atoi(v)); break;
		// low date <date l="20100908" h="20110101"/> 
		case 'l': pat.setLo(atoi(v)); break;
		// past <date p="yes"/> or <date past="y"/>
		case 'p': pat.setPast(); break;
        case 'x': 
            pat.setMatchModeFromAttribute(v);
            break;
		}
	}	
	statement.pushNode( BTND_PatternData( pat));
}

DEFINE_BELParserXML_taghandle(TIME)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Pattern_Time pat; 
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'l':  // l - lo HHMMSS
			pat.setLo( atoi(v) );
			break;
		case 'h':  // l - lo HHMMSS
			pat.setHi( atoi(v) );
			break;
		case 'f': pat.setFuture(); break;
		case 'p': pat.setPast(); break;
        case 'x': 
            pat.setMatchModeFromAttribute(v);
            break;
		}
	}
	statement.pushNode( BTND_PatternData( pat));
}

DEFINE_BELParserXML_taghandle(RX)
{}
DEFINE_BELParserXML_taghandle(TDRV)
{}
DEFINE_BELParserXML_taghandle(WCLS)
{}
/// W - wildcard is a pattern for very wide matches - be careful
// if no attributes are supplied w matches any bead 
// otherwise e - anything entity related (erc, ent) 
// 
DEFINE_BELParserXML_taghandle(W)
{
	if( close ) {
		statement.popNode();
		return;
	}
    
	BTND_Pattern_Wildcard pat; 
    const char* modeStr = "a";
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
        case 'm':  // mode 
            modeStr= v;
            break;
        }
    }
    pat.setFromAttr(modeStr);
	statement.pushNode( BTND_PatternData( pat));
}

void BELParserXML::processAttrForStructTag( BTND_StructData& dta, const char_cp * attr, size_t attr_sz )
{
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'v': 
			dta.setVarId( internString_internal(v) );
			return;
		}
	}
}
DEFINE_BELParserXML_taghandle(EXPAND)
{
	if( close ) { return; }

	const char  *macroName      = 0,
	            *varName        = 0,
                *trieClassName  = 0,
                *trieName       = 0;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'a':
		    if (!strcmp(n, "as"))
		        varName = v;
		    break;
		case 'c': trieClassName= v; break; // trie class 
		case 'n': macroName= v; break;
        case 't': trieName = v; break;     // trie name
		}
	}
     
    // StoredUniverse* uni = reader->getCurrentUniverse();
	const BELParseTreeNode* macroNode = 0;
    if( trieName ) {
        const GlobalPools& gp = reader->getGlobalPools();
        uint32_t trieName_id = internString_internal(trieName); 
        uint32_t trieClassName_id = 0xffffffff;
        if( trieClassName ) {
            if( !strcmp(trieClassName,"generic") ) 
                trieClassName="";

            trieClassName_id = internString_internal(trieClassName);
        } else {
            trieClassName_id = reader->getTrie().getTrieClass_strId();
        }
        const BELTrie* trie = gp.getTrie(trieClassName_id, trieName_id);
        if( trie ) {
	        macroNode = getMacroByName(*trie,macroName);
        } else {
            BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
		    errStream.os << "macro " << 
                gp.internalString_resolve(trieClassName_id) << "." << 
                gp.internalString_resolve(trieName_id) << "." << 
                macroName  << " doesnt exist";
        }
    } else
	    macroNode = getMacroByName(macroName);

	if( macroNode ) {

		BELParseTreeNode* curNode = statement.getCurTreeNode();
		BELParseTreeNode &n = curNode->addChild(*macroNode);
		if (varName) {
		    // safe as the root of any pattern is always a list
		    BTND_StructData &sd = boost::get<BTND_StructData>(n.getNodeData());
		    sd.setVarId(internString_internal(varName));
		}
	} else {
        BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
		// BarzXMLErrorStream errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
		errStream.os << "macro " << macroName  << " doesnt exist";
	}
}

DEFINE_BELParserXML_taghandle(LIST)
{
	if( close ) { statement.popNode(); return; }

	BTND_StructData node( BTND_StructData::T_LIST);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
DEFINE_BELParserXML_taghandle(ANY)
{
	if( close ) { statement.popNode(); return; }

	BTND_StructData node( BTND_StructData::T_ANY);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
DEFINE_BELParserXML_taghandle(OPT)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_OPT);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
DEFINE_BELParserXML_taghandle(PERM)
{
	if( close ) {
		statement.popNode();
		return;
	}
    int structType = BTND_StructData::T_PERM;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 't': // t or type (optional attribute can be FLIP, other types to come)
            if( !n[1] || !strcasecmp(n,"type") ) {
                if( v[0] == 'f' ) {
                    if( !v[1] || !strcasecmp(v,"flip")  ) 
                        structType = BTND_StructData::T_FLIP;
                }
            }
            break;
        }
    }

	BTND_StructData node( structType );
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode( node );
}
DEFINE_BELParserXML_taghandle(SUBSET)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_SUBSET);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode(node);
}
DEFINE_BELParserXML_taghandle(TAIL)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_StructData node( BTND_StructData::T_TAIL);
	processAttrForStructTag( node, attr, attr_sz );
	statement.pushNode(node);
}
/// rewrite tags 
DEFINE_BELParserXML_taghandle(LITERAL)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Literal literal;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 't': // type
			switch( v[0] ) {
			case 'f': // t="fluff" ~ stop token
			case 's': // t="stop" ~ stop token
				literal.setStop();
				break;
			case 'c': // t="c:ALIASXXX" ~ compounded word with alias ALIASXXX
                {
				const char* semicolon = strchr( v, ':' );
				uint32_t tokId = addCompoundedWordLiteral( semicolon );
				literal.setCompound(tokId);
                }
				break;
			}
			break;
        case 'i': // INTERNAL ONLY STRING
            literal.setInternalString();
            break;
		}
	}
	statement.pushNode( BTND_RewriteData(literal) );
}

DEFINE_BELParserXML_taghandle(NV)
{
    if( close ) {
        return;
    }
    const char* name = 0;
    const char* value = 0;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
        switch( n[0] ){
        case 'n':
            name = v;
            break;
        case 'v':
            value = v;
            break;
        }
    }
    if( name && value ) {
        StoredUniverse* uni = reader->getCurrentUniverse();
        if( uni ) {
            uni->getGhettodb().store( statement.getCurEntity(), name, value );
        } else {
            reader->getErrStreamRef() << "<error> NV tags ignored without universe set</error>\n";
        }
    }
}

namespace
{
	bool parseComma(const char *coordStr, std::pair<double, double>& out)
	{
		std::istringstream istr(coordStr);
		istr >> out.first;
		char sep;
		istr >> sep;
		if (sep != ',')
			return false;
		istr >> out.second;
		return !istr.fail();
	}

	bool parseCoord(const char *coordStr, std::pair<double, double>& out)
	{
		return parseComma(coordStr, out);
	}
}

DEFINE_BELParserXML_taghandle(MKENT)
{
	if( close ) {
        if( !statement.isBlank() ) 
		    statement.popNode();
        else 
            statement.clearCurEntity();
		return;
	}
	BTND_Rewrite_MkEnt mkent;
	uint32_t eclass = 0, subclass = 0, topicClass = 0, topicSubclass=0;
	const char *idStr = 0;
	const char *topicIdStr = 0;
    int relevance = 0, topicRelevance = 0;
	const char *canonicName = 0, *topicCanonicName = 0;
	const char *rawCoord = 0;
    uint32_t topicStrength = 0;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'c': eclass =  atoi(v); break;
		case 's': subclass =  atoi(v); break;
		case 'i': idStr = v; break;
        
		case 'n': canonicName = v; break;       // canonic name of the entity if its an empty string getDescriptiveNameFromPattern_simple will be used
		case 'r': relevance = atoi(v); break;   // relevance of the entity 
		case 't': {
            switch(n[1]) {
                case 'c': topicClass =  atoi(v); break;
                case 'g': topicStrength = atoi(v); break;
                case 'i': topicIdStr = v; break;
                case 'n': topicCanonicName = v; break;
                case 'r': topicRelevance = atoi(v); break;
                case 's': topicSubclass =  atoi(v); break;
            }
            break;
        }
		case 'p': rawCoord = v; break;
		} // n[0] switch
	}

	//const StoredEntity& ent  = reader->getUniverse().getDtaIdx().addGenericEntity( idStr, eclass, subclass );
    GlobalPools& gp = reader->getGlobalPools();
    uint32_t idStrId = ( idStr ? reader->getGlobalPools().internString_internal(idStr) : 0xffffffff );
	const StoredEntity& ent  = gp.getDtaIdx().addGenericEntity( idStrId, eclass, subclass );
    StoredUniverse* universe = getReader()->getCurrentUniverse();
    const uint32_t curUserId = universe->getUserId();

	if (rawCoord)
	{
		std::pair<double, double> coords;
		if (parseCoord(rawCoord, coords)) {
			if( universe ) 
                universe->getGeo()->addEntity(ent, coords);
		} else
			AYLOG(ERROR) << "failed to parse coords for entity "
					<< ent.getClass() << " "
					<< ent.getSubclass() << " "
					<< (canonicName ? canonicName : "<no name>");
	}
	
	BarzelTranslationTraceInfo traceInfo;
	traceInfo.source = statement.stmt.getSourceNameStrId();
	traceInfo.statementNum = statement.stmt.getStmtNumber();
	traceInfo.emitterSeqNo = 0;
	universe->getEntRevLookup().add(StoredEntityUniqId(idStrId, eclass, subclass), traceInfo);
    
	mkent.setEntId( ent.entId );
    statement.setCurEntity( ent.getEuid() );
    if( statement.hasPattern() || statement.isProc() ) {
        /// 
	    statement.pushNode( BTND_RewriteData(mkent));
    }
    bool isTrivialRewrite = ( statement.stmt.translation.getTrivialRewriteData() != 0 );
    /// adding deduced entity name 
    if( isTrivialRewrite &&  !reader->is_noCanonicalNames() ) {
        EntityData::EntProp* eprop = ( universe ? universe->getEntPropData(ent.getEuid()) : gp.getEntPropData(ent.getEuid()) );

        if( !eprop || canonicName /*|| !eprop->is_nameExplicit()*/ ) {
            std::string theName;
            if( !canonicName || !*canonicName ) {
                if( statement.hasPattern() ) {
                    statement.stmt.pattern.getDescriptiveNameFromPattern_simple( theName, gp ) ;
                }
            } else 
                theName.assign(canonicName);
            
            if( universe&& curUserId )
                eprop= universe->setEntPropData( ent.getEuid(), theName.c_str(), relevance, (canonicName!=0) );
            else 
                eprop= gp.setEntPropData( ent.getEuid(), theName.c_str(), relevance, (canonicName!=0) );

            if( canonicName && eprop ) 
                eprop->set_nameExplicit();
        }
    }
    if( topicClass ) {
        uint32_t topicIdStrId = reader->getGlobalPools().internString_internal(topicIdStr) ;
	    const StoredEntity& topicEnt  = gp.getDtaIdx().addGenericEntity( topicIdStrId, topicClass, topicSubclass);

        BELTrie& trie = reader->getTrie();
        trie.linkEntToTopic( topicEnt.getEuid(), ent.getEuid(), topicStrength );
        if( topicRelevance || topicCanonicName ) {
            EntityData::EntProp* eprop = ( universe ? universe->getEntPropData(ent.getEuid()) : gp.getEntPropData(ent.getEuid()) );
            if( topicCanonicName && eprop ) 
                eprop->set_nameExplicit();
        }
    }
}
        

DEFINE_BELParserXML_taghandle(BLOCK)
{
	if( close ) {
		statement.popNode();
		return;
	}
    BTND_Rewrite_Control ctrl;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
        case 'c':{
                char c0=v[0]; 
                switch( c0 ) {
                case 'c': // c(omma) comma
                    ctrl.setCtrl( BTND_Rewrite_Control::RWCTLT_COMMA );
                    break;
                case 'v': // var
                    ctrl.setCtrl( BTND_Rewrite_Control::RWCTLT_VAR_GET );
                }
            }
            break;
        case 'r':{ /// variable
            if( *v == 'y' ) 
                ctrl.setVarModeRequest();
            }
            break;
        case 'v':{ /// variable
            uint32_t varId = reader->getGlobalPools().internString_internal(v) ;
            ctrl.setVarId( varId );
            }break;
            
        }
    }
	statement.pushNode( BTND_RewriteData(ctrl));
}
DEFINE_BELParserXML_taghandle(RNUMBER)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Number num;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		switch( n[0] ) {
		case 'v': {
			if( strchr( v, '.' ) ) {
				num.set( atof( v ) );
			} else {
				num.set( atoi( v ) );
			}
		}
			break;
		}
	}
	statement.pushNode( BTND_RewriteData( num));
}
DEFINE_BELParserXML_taghandle(VAR)
{
    if( statement.isState_Pattern() ) {
        taghandle_LIST(TAG_VAR,attr,attr_sz,close);
        return;
    }

	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Variable var;
    const char* varName = 0;
    bool isReqVar = false;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value

		switch( n[0] ) {
		case 'a': { // <var arg="1"/> positional argument (for proc calls)
			int num  = atoi(v);
			if( num >= 0 ) 
				var.setPosArg(num);
		}
			break;
		case 'g': { // <var gn="1"/> gap number - if pattern is a * b c * d gn[1] is a, gn[2] = (b c), gn[3] = 3 
			int num  = atoi(v);
			if( num > 0 ) 
				var.setWildcardGapNumber( num );
		}
			 break;
		case 'n': { // <var name=""/> - named variable
			/// intern
			var.setVarId( internVariable(v) );
            varName = v;
		}
			break;
		case 'p': { // <var pn="1"/> - pattern element number - if pattern is "a * b * c" then pn[1] is a, pn[2] is the first * etc
					// pn[0] is the whole sub chain matching this pattern 
			int num  = atoi(v);
			if( num > 0 ) 
				var.setPatternElemNumber( num );
			else {
                // BarzXMLErrorStream errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
                BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
				errStream.os << "invalid pattern element number " << v ;
			}
		}
			break;
		case 'r': { /// r - variable is a request var
            if( v && *v == 'y' ) {
                isReqVar = true;
            }
        }
            break;
		case 'w': { // <var w="1"/> - wildcard number like $1 
			int num  = atoi(v);
			if( num > 0 ) 
				var.setWildcardNumber( num );
			else {
				// BarzXMLErrorStream errStream(reader->getErrStreamRef(),statement.stmt.getStmtNumber());
                BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
                errStream.os << "invalid wildcard number " << v ;
			}
		}
			break;

		}
	}
    if( isReqVar && varName ) {
        var.setRequestVarId( reader->getGlobalPools().internString_internal(varName) );
    }
	statement.pushNode( BTND_RewriteData( var));
}
DEFINE_BELParserXML_taghandle(GET)
{
    // alias
    taghandle_SET(TAG_GET,attr, attr_sz, close);
}
DEFINE_BELParserXML_taghandle(SET)
{
	if( close ) { statement.popNode(); return; }
	BTND_Rewrite_Function f;
    uint32_t funcNameId = 0xffffffff;
    funcNameId = reader->getGlobalPools().internString_internal( (tid == TAG_SET ? "set": "get") ) ;
    f.setNameId( funcNameId ) ;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		if( *n == 'a' ) {
            f.setArgStrId( reader->getGlobalPools().internString_internal(v) );
        } else if( *n == 'v' ) { // variable 
            f.setVarId( reader->getGlobalPools().internString_internal(v) );
        }
	}

    statement.pushNode( BTND_RewriteData(f));
}
DEFINE_BELParserXML_taghandle(FUNC)
{
	if( close ) {
		statement.popNode();
		return;
	}
	BTND_Rewrite_Function f;
    uint32_t funcNameId = 0xffffffff;
	for( size_t i=0; i< attr_sz; i+=2 ) {
		const char* n = attr[i]; // attr name
		const char* v = attr[i+1]; // attr value
		if( *n == 'n' ) {
            funcNameId = reader->getGlobalPools().internString_internal(v) ;
		    f.setNameId( funcNameId ) ;
		} else if( *n == 'a' ) {
            f.setArgStrId( reader->getGlobalPools().internString_internal(v) );
        } else if( *n == 'v' ) { // variable 
            f.setVarId( reader->getGlobalPools().internString_internal(v) );
        } else if( *n == 'r' ) { // request variable RequestEnvironment::d_reqVar
            if( *v == 'y' ) 
                f.setVarModeRequest();
        }
	}
    if( funcNameId != 0xffffffff ) 
	    statement.pushNode( BTND_RewriteData( f));
    else { // no valid function named 
        reader->getErrStreamRef() << "<error stmt=\"" << statement.stmt.getStmtNumber() << "\">" << " invalid function in translation</error>\n";

        BTND_Rewrite_Literal literal;
        literal.setStop();
        statement.pushNode( BTND_RewriteData(literal) );
    }
}

DEFINE_BELParserXML_taghandle(SELECT)
{
    if( close ) {
        statement.popNode();
        return;
    }
    uint32_t varId= 0xffffffff;
    for( size_t i=0; i< attr_sz; i+=2 ) {
        const char* n = attr[i]; // attr name
        const char* v = attr[i+1]; // attr value
        switch(n[0]) {
        case 'v':
            varId = internVariable(v);
            break;
        }
    }
    statement.pushNode( BTND_RewriteData(BTND_Rewrite_Select(varId)) );
}

DEFINE_BELParserXML_taghandle(CASE)
{
    if( close ) {
        statement.popNode();
        return;
    }
    uint32_t caseId = 0xffffffff;
    for( size_t i=0; i< attr_sz; i+=2 ) {
        const char* n = attr[i]; // attr name
        const char* v = attr[i+1]; // attr value
        switch(n[0]) {
        case 'l':
            caseId = internString(LANG_UNKNOWN,v,true,0).getStringId();
            break;
        case 'p':
            caseId = (uint32_t)v[0]; // this is a hack really
            break;
        }
    }
    statement.pushNode( BTND_RewriteData(BTND_Rewrite_Case(caseId)) );
}


void BELParserXML::processLogic( int tid , bool close = false)
{
    if (close) {
      statement.popNode();
      return;
    }
    BTND_Rewrite_Logic l;
    switch(tid) {
    case TAG_AND: l.setType(BTND_Rewrite_Logic::AND); break;
    case TAG_OR: l.setType(BTND_Rewrite_Logic::OR); break;
    case TAG_NOT:
        l.setType(BTND_Rewrite_Logic::NOT);
        break;
    }
    statement.pushNode(BTND_RewriteData(l));
}

void BELParserXML::endElement( const char* tag )
{
    if( cdataBuf.length() ) {
	    BELParseTreeNode* node = statement.getCurrentNode();
	    if( node ) 	{
		    boost::apply_visitor( BTND_text_visitor(*this,cdataBuf.c_str(),cdataBuf.length(),node->noTextToNum), node->getVar() ) ; 
	    }
    }

	int tid = getTag( tag );
    
	elementHandleRouter(tid,0,0,true);
	if( !tagStack.empty() ) 
		tagStack.pop();

    cdataBuf.clear();
}

void BELParserXML::getElementText( const char* txt, int len )
{
	if( tagStack.empty() ) {
        cdataBuf.clear();
		return; // this should never happen 
    }
    
    cdataBuf.append( txt, len );
	return;
}
BELParserXML::~BELParserXML()
{
	if( parser ) 
		XML_ParserFree(parser);
}

int BELParserXML::parse( std::istream& fp )
{
	const char* srcName =reader->getInputFileName().c_str();
	uint32_t srcNameStrId = 0xffffffff; // will be set in escapeForXmlAndIntern
    
    std::string srcNameCpp;
    escapeForXmlAndIntern( reader->getGlobalPools(), srcNameStrId, srcNameCpp, srcName );
	statement.stmt.setSrcInfo(srcName, srcNameStrId );
	//statement.stmt.setSrcInfo(reader->getInputFileName().c_str());

	/// initialize parser  if needed
	if( !parser ) {
		parser = XML_ParserCreate(NULL);
		if( !parser ) {
            BarzXMLErrorStream  errStream(reader->getErrStreamRef());
			errStream.os << "BELParserXML couldnt create xml parser";
			return 0;
		}
	} else 
		XML_ParserReset(parser,0);
	
	XML_SetUserData(parser,this);
	XML_SetElementHandler(parser, 
		::startElement, ::endElement);
	XML_SetCharacterDataHandler(parser, charDataHandle);
	
	char buf[ 128*1024 ];	
	bool done = false;
    const size_t buf_len = sizeof(buf)-1;
	do {
    	fp.read( buf,buf_len );
    	size_t len = fp.gcount();
    	done = len < buf_len;
    	if (!XML_Parse(parser, buf, len, done)) {
      		fprintf(stderr,
	      		"%s at line %d\n",
	      		XML_ErrorString(XML_GetErrorCode(parser)),
	      		(int)XML_GetCurrentLineNumber(parser));
      		return 1;
    	}
	} while (!done);
	return 0;
}

#define CHECK_1CW(N)  if( !s[1] ) return N;
#define CHECK_2CW(c,N)  if( c[0] == s[1] && !s[2] ) return N;
#define CHECK_3CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && !s[3] ) return N;
#define CHECK_4CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && c[2] == s[3] && !s[4] ) return N;
// needed for stmset
#define CHECK_5CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && c[2] == s[3] && c[3] == s[4] && !s[5] ) return N;
#define CHECK_6CW(c,N)  if( c[0] == s[1] && c[1] == s[2] && c[2] == s[3] && c[3] == s[4] && c[4] == s[5] && !s[6] ) return N;

int BELParserXML::getTag( const char* s ) const
{
	switch( *s ) {
	case 'a':
	CHECK_3CW("ny",TAG_ANY) // <any>
	CHECK_3CW("nd",TAG_AND) // <and>
		break;
	case 'b':
	CHECK_5CW("lock", TAG_BLOCK ) // <case>
	    break;
	case 'c':
	CHECK_4CW("ase", TAG_CASE ) // <case>
	CHECK_4CW("ond", TAG_COND ) // <cond>
	    break;
	case 'd':
	CHECK_4CW("ate", TAG_DATE ) // <date> 
	CHECK_4CW("tim", TAG_DATETIME ) // <dtim> 
		break;
	case 'e':
	CHECK_3CW("nt",TAG_ENTITY) // <ent>
	CHECK_3CW("rc",TAG_ERC) // <erc>
	CHECK_6CW("rcexp",TAG_ERCEXPR) // <ercexp>
	CHECK_6CW("xpand",TAG_EXPAND) // <expand>
		break;
	case 'f':
	CHECK_4CW("unc", TAG_FUNC ) // <func>
		break;
	case 'g':
	CHECK_3CW("et", TAG_GET ) // <get>
		break;
	case 'l':
	CHECK_4CW("ist", TAG_LIST ) // <list>
	CHECK_4CW("trl", TAG_LITERAL ) // <ltrl>
		break;
	case 'm':
	CHECK_5CW("kent", TAG_MKENT) // <n>
		break;
	case 'n':
	CHECK_1CW(TAG_N) // <n>
	CHECK_2CW("v",TAG_NV) // <n>
	CHECK_3CW("ot",TAG_NOT) // <not>
		break;
	case 'o':
	CHECK_3CW("pt", TAG_OPT ) // <opt>
	CHECK_2CW("r", TAG_OR ) // <or>
		break;
	case 'p':
	CHECK_1CW(TAG_P) // <p> 
	CHECK_3CW("at",TAG_PATTERN ) // <pat>
	CHECK_4CW("erm",TAG_PERM ) // <perm>
		break;
	case 'r': 
	CHECK_2CW("n",TAG_RNUMBER) // <rn>
	CHECK_5CW("ange",TAG_RANGE) // <range>
		break;
	case 's':
	CHECK_3CW("et",TAG_SET ) // <set>
	CHECK_4CW("tmt",TAG_STATEMENT )  // <stmt>
	CHECK_6CW("tmset",TAG_STMSET )  // <stmset>
	CHECK_6CW("elect",TAG_SELECT )  // <select>
	CHECK_6CW("ubset",TAG_SUBSET )  // <subset>
		break;
	case 't':
	CHECK_1CW(TAG_T) // <t>
	CHECK_4CW("ran",TAG_TRANSLATION) // <tran>
	CHECK_4CW("ime",TAG_TIME) // <time>
    CHECK_4CW("ail",TAG_TAIL) // <tail>
    CHECK_4CW("est",TAG_TEST) // <test>

	CHECK_2CW("g",TAG_TG) // <tg>
	CHECK_4CW("drv",TAG_TDRV) // <tdrv>

		break;
	case 'v':
	CHECK_3CW("ar",TAG_VAR) // <var>
		break;
	case 'w':
	CHECK_1CW(TAG_W) // <w>
	CHECK_4CW("cls",TAG_WCLS) // <wcls>
        break;
	default:
		break;
	} // switch ends
	
    {
        BarzXMLErrorStream errStream( *reader, statement.stmt.getStmtNumber());
        errStream.os << "Unknown tag " << s;
    }
	return TAG_UNDEFINED;
}

void BELParserXML::CurStatementData::clear()
{
	bits.clear();
	procNameId = 0xffffffff;
	macroNameId=0xffffffff;

    /// sets blank and clears current entity
	setBlank(); 

    stmt.clearUnmatchable();
	stmt.translation.clear();
	stmt.pattern.clear();
	if ( !nodeStack.empty() ) {
		AYDEBUG(nodeStack.size());
		while (!nodeStack.empty())
			nodeStack.pop();
	}
}




} // barzer namespace ends
