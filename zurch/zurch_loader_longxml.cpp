#include <zurch/zurch_loader_longxml.h>
#include <zurch_settings.h>
#include <ay/ay_logger.h>
#include <ay/ay_util_time.h>
#include <ay/ay_tag_markup_parser.h>
extern "C" {
#include <expat.h>
}

namespace zurch {
namespace {

enum {
    TAG_INVALID,
    TAG_ROOT,
    TAG_TABLE,
    /// add new tags above this line only
    TAG_MAX
};

#define DECL_TAGHANDLE(X) inline int xmlth_##X( ZurchLongXMLParser& parser, int tagId, const char* tag, const char**attr, size_t attr_sz, bool open)

#define IS_PARENT_TAG(X) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X) )
#define IS_PARENT_TAG2(X,Y) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || parser.tagStack.back()== TAG_##Y) )
#define IS_PARENT_TAG3(X,Y,Z) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || \
    parser.tagStack.back()== TAG_##Y || parser.tagStack.back()== TAG_##Z) )

#define ALS_BEGIN    const char** attr_end = attr+ attr_sz;\
	for( const char** a=attr; a< attr_end; a+=2 ) {\
        const char* n = 0[ a ];\
        const char* v = 1[ a ]; \
        switch(toupper(n[0])) {

#define ALS_END }}

enum {
    TAGHANDLE_ERROR_OK,
    TAGHANDLE_ERROR_PARENT, // wrong parent 
    TAGHANDLE_ERROR_ENTITY, // misplaced entity 
};

/// wrapper object for expat
struct ZurchLongXMLParser_anon {
    XML_ParserStruct*    expat;
    ZurchLongXMLParser&   parser;
    ZurchLongXMLParser_anon( ZurchLongXMLParser& p ) : expat(0),parser(p) {}
    ~ZurchLongXMLParser_anon(); 
    ZurchLongXMLParser_anon& init();
    void parse( std::istream& fp );
};

void tagRouter( ZurchLongXMLParser& parser, const char* t, const char** attr, size_t attr_sz, bool open);

extern "C" {

// cast to XML_StartElementHandler

static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n;
	const char** atts = (const char**)a; 
	ZurchLongXMLParser_anon *loader =(ZurchLongXMLParser_anon *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( loader->expat );
	if( attrCount <=0 || (attrCount & 1) )
		attrCount = 0; // odd number of attributes is invalid

    size_t lineNumber = XML_GetCurrentLineNumber(loader->expat);
	tagRouter( loader->parser, name, atts, attrCount, true );
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	ZurchLongXMLParser_anon *loader =(ZurchLongXMLParser_anon *)ud;
	tagRouter( loader->parser, name, 0, 0, false );
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
    if( !len )
        return;
	const char* s = (const char*)str;
	ZurchLongXMLParser_anon *loader =(ZurchLongXMLParser_anon *)ud;
}
} // extern "C" ends
ZurchLongXMLParser_anon& ZurchLongXMLParser_anon::init()
{
    if( !expat ) 
        expat= XML_ParserCreate(NULL);
    else 
        XML_ParserReset(expat,0);

    XML_SetUserData(expat,this);
    XML_SetElementHandler(expat, startElement, endElement);
    XML_SetCharacterDataHandler(expat, charDataHandle);

    return *this;
}

ZurchLongXMLParser_anon::~ZurchLongXMLParser_anon()
{
    if(expat) 
        XML_ParserFree(expat);
}
void  ZurchLongXMLParser_anon::parse( std::istream& fp )
{
    if(!expat) {
        AYLOG(ERROR) << "invalid expat instance\n";
    }
	char buf[ 1024*1024 ];	
	bool done = false;
    const size_t buf_len = sizeof(buf)-1;
	do {
    	fp.read( buf,buf_len );
    	size_t len = fp.gcount();
    	done = len < buf_len;
    	if (!XML_Parse(expat, buf, len, done)) {
      		fprintf(stderr, "%s at line %d\n", XML_ErrorString(XML_GetErrorCode(expat)), (int)XML_GetCurrentLineNumber(expat));
      		return;
    	}
	} while (!done);
}


/// handlers
DECL_TAGHANDLE(ROOT) {
    ALS_BEGIN /// attributes
    ALS_END  /// end of attributes
    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(TABLE) {
    if( !IS_PARENT_TAG(ROOT) ) 
        return TAGHANDLE_ERROR_PARENT;
    #define CASEIF(x) if( !strcasecmp(#x,n) ) parser.d_data.d_##x.assign(v);
    if( open ) {
        ALS_BEGIN // attributes  loop
            case 'B':
                CASEIF(BegDate)
                break;
            case 'C': 
                CASEIF(Content)
                break;
            case 'D': 
                CASEIF(DocName)
                break;
            case 'I': 
                CASEIF(ID)
                break;
            case 'K': 
                CASEIF(Keywords)
                break;
            case 'M': 
                CASEIF(ModuleID)
                break;
				/* there is some bullshit with the new keywords in the file so we don't parse it for now
            case 'N': 
                CASEIF(NewKeywords)
                break;
				*/
            case 'R': 
                CASEIF(Rubrics)
                break;
            case 'S': 
                CASEIF(SortName)
                break;
        ALS_END  // end of attributes
        ++parser.d_numCallbacks;
        if( !(parser.d_numCallbacks%500) )
            std::cerr << "."; // trace

        parser.callback();
    } else { /// closing TABLE
    }

    return TAGHANDLE_ERROR_OK;
}

#define SETTAG(X) (handler=((tagId=TAG_##X),xmlth_##X))
void tagRouter( ZurchLongXMLParser& parser, const char* t, const char** attr, size_t attr_sz, bool open)
{
    char c0= toupper(t[0]),c1=(c0?toupper(t[1]):0),c2=(c1?toupper(t[2]):0),c3=(c2? toupper(t[3]):0);
    typedef int (*TAGHANDLE)( ZurchLongXMLParser& , int , const char* , const char**, size_t, bool );
    TAGHANDLE handler = 0;
    int       tagId   =  TAG_INVALID;
    switch( c0 ) {
    case 'T': if( !strcasecmp(t,"table") )     SETTAG(TABLE); break;
    case 'R': if( !strcasecmp(t,"root") )     SETTAG(ROOT); break;
    }
    if( tagId != TAG_INVALID ) {
        if( !open ) {
            if( parser.tagStack.size() )
                parser.tagStack.pop_back();
        } 
        if( handler )
            handler(parser,tagId,t,attr,attr_sz,open);

        if( open ) {
            parser.tagStack.push_back( tagId );
        }
    }
}

} // anonymous namespace 

int ZurchLongXMLParser::callback()
{
    std::cout << d_data.d_ModuleID << ":" << d_data.d_ID << "|" << d_data.d_DocName << std::endl;
    return 0;
}

namespace {
inline const char* string_has_junk( const char* str, size_t s_sz ) 
{
    for( const char*s =str, *s_end = str+s_sz; s< s_end; ++s ) {
        uint8_t c = static_cast<uint8_t>( *s );
        if( !isascii(c) ) {
            if( s +1 == s_end ) 
                return s;
            if( c == 0xd0 || c== 0xd1 ) {
                if( isascii(s[1]) ) 
                    return s;
                ++s;
            } else if( !isascii(c) ) 
                return s;
        }
    }
    return 0;
}
struct PhraseizerCB {
    std::ostream& d_fp;
    PhraseBreaker& d_phraser;
    std::string d_docId;

    enum txt_type_t { 
        TXT_CONTENT,
        TXT_NAME,
        TXT_KEYWORDS,
		TXT_NEWKEYWORDS,
		TXT_RUBRICS
    };
    txt_type_t d_txtType; // TXT_XX
    size_t d_fragment;
    std::string d_tmp;

    PhraseizerCB( std::ostream& fp, PhraseBreaker& pb) : 
        d_fp(fp), d_phraser(pb), d_txtType(TXT_CONTENT), d_fragment(0)  {}
    PhraseizerCB( std::ostream& fp, PhraseBreaker& pb, const std::string& docId, txt_type_t tt ) : 
        d_fp(fp), d_phraser(pb), d_txtType(tt), d_fragment(0) {}

    void operator()( const ay::xhtml_parser_state& state, const char* s, size_t s_sz )
    {

        if( state.isCallbackText() ) {
            std::string tmp(s, s_sz);
            ay::html::unescape_in_place(tmp);
            ay::unicode_normalize_punctuation(tmp);
            d_phraser.breakBuf( *this, tmp.c_str(), tmp.length() );
        }

    }
    void operator()( PhraseBreaker& pz, const char* str, size_t sz )
    {

        size_t numNonJunk = 0;
        const char* cleanS = str;
        bool sawGoodChar = false;
        for( const char* s = str, *s_end = str+sz; numNonJunk< 4 && s< s_end; ++s ) {
            if( isspace(*s) || ispunct(*s) || (isascii(*s) && !isprint(*s)) ) {
                if( !sawGoodChar )
                    ++cleanS;
            } else  if( isascii(*s) ) {
                sawGoodChar= true;
                numNonJunk+=2;
            } else {
                sawGoodChar= true;
                ++numNonJunk;
            }
        }
        if( numNonJunk< 4) 
            return;

        size_t cleanS_sz = sz - (cleanS-str); 

        (d_fp << d_docId << "|T" << d_txtType << "|" << d_fragment++ << "|").write( cleanS, cleanS_sz) << std::endl;
    }

    void setTextType( txt_type_t t ) 
    {
        d_txtType = t;
        d_fragment = 0;
    }
    void parseText( const std::string& str , txt_type_t t, const char* spc=0 ) 
    {
        std::stringstream sstr;
        sstr << str;
        if( spc ) 
            sstr << spc;

        ay::xhtml_parser<PhraseizerCB> parser( sstr , *this );
        setTextType( t );
        parser.setMode(ay::XHTML_MODE_HTML);
        parser.parse();
    }
};

} // anonymous namespace 

int ZurchLongXMLParser_Phraserizer::callback()
{
    PhraseizerCB pz( d_outFP, d_phraser );
    pz.d_docId = d_data.d_ModuleID + "." + d_data.d_ID;
    if( d_mode==MODE_PHRASER ) {
        // NAME
        pz.parseText( d_data.d_DocName , PhraseizerCB::TXT_NAME,     " " );
        pz.parseText( d_data.d_Content , PhraseizerCB::TXT_CONTENT );
        pz.parseText( d_data.d_Keywords, PhraseizerCB::TXT_KEYWORDS, " " );
        pz.parseText( d_data.d_NewKeywords, PhraseizerCB::TXT_NEWKEYWORDS, " " );
        pz.parseText( d_data.d_Rubrics, PhraseizerCB::TXT_RUBRICS, " " );
    } else if( d_mode==MODE_EXTRACTOR ) {
        d_outFP << "TITLE|" << pz.d_docId << "|" << d_data.d_DocName << std::endl;
        d_outFP << "C_BEGIN|" << pz.d_docId << std::endl << d_data.d_Content << std::endl;
        d_outFP << "C_END|" << pz.d_docId <<std::endl ;
    }
    return 0;
}
#define WEIGHT_BOOST(x) ZurchModelParms::get().d_section.d_WEIGHT_BOOST_##x

void ZurchLongXMLParser_DocLoader::loadingDoneCallback()
{
    d_loader.index().d_docDataIdx.simpleIdx().sort();
}
int ZurchLongXMLParser_DocLoader::callback()
{
    std::string docName = d_data.d_ModuleID + "." + d_data.d_ID;
    uint32_t docId = d_loader.addDocName( docName.c_str() );
    
    if(d_data.d_DocName.length() )
        d_loader.setDocTitle( docId, d_data.d_DocName ); 

    
    { // NAME
        std::stringstream sstr;
        sstr << d_data.d_DocName << " ";
        d_loader.setCurrentWeight(WEIGHT_BOOST(NAME));
		d_loader.index().setConsiderFeatureCount(false);
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
		d_loader.index().setConsiderFeatureCount(true);
    }
    { // KEYWORDS
        std::stringstream sstr;
        sstr << d_data.d_Keywords << " ";
        d_loader.setCurrentWeight(WEIGHT_BOOST(KEYWORD));
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
    }
    { // KEYWORDS
        std::stringstream sstr;
        sstr << d_data.d_Rubrics << " ";
        d_loader.setCurrentWeight(WEIGHT_BOOST(RUBRIC));
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
    }
    { /// CONTENT
        std::stringstream sstr;
        sstr << d_data.d_Content;
        d_loader.setCurrentWeight(WEIGHT_BOOST(NONE));
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
        if( d_loader.hasContent() ) 
            d_loader.addDocContents( docId, d_data.d_Content );
    }
    /// adding module info
    {
        int module = atoi( d_data.d_ModuleID.c_str() );
        d_loader.index().d_docDataIdx.simpleIdx().Int.append( "module", docId, module );
    }
    return 0;
}

namespace {

struct phrase_fields_t {
    enum {
        REC_TYPE_UNKNOWN,
        REC_TYPE_CONTENT,
        REC_TYPE_TITLE,
        REC_TYPE_KEYWORD,
        REC_TYPE_RUBRIC,

        /// add new types above
        REC_TYPE_MAX
    };
    int recType;
    size_t phraseNum;
    DocFeatureLink::Weight_t weight;
    char* name;
    char* text;
	bool considerCount;

    phrase_fields_t(): 
        recType(REC_TYPE_UNKNOWN), phraseNum(0),
        weight(WEIGHT_BOOST(NONE)), name(0), text(0), considerCount(true) {}
    void clear() 
    {
        phraseNum = 0;
        recType= REC_TYPE_UNKNOWN;
        weight = WEIGHT_BOOST(NONE);
        name = text = 0;
		considerCount = true;
    }
}; 


int phrase_fmt_get_fields( ZurchPhrase_DocLoader& loader, phrase_fields_t& flds, char* buf, size_t buf_sz, const std::string & docTitle )
{
    flds.clear();
    const char* buf_end = buf+buf_sz;
    char* tok = buf;
    char* pipe = strchr( tok, '|' );
    if( !pipe ) 
        return 1;
    *pipe = 0;
    flds.name = tok;

    /// weight 
    tok = ( pipe < buf_end ? pipe +1 : 0 );
    pipe = ( tok < buf_end ? strchr( tok, '|' ) : 0 );
    double adjFactor = 1.0;
    if( !pipe ) return 1;
    if( toupper(tok[0]) =='T' ) {
        switch( tok[1] ) {
        case '0': 
            flds.weight = WEIGHT_BOOST(NONE); 
            flds.recType = phrase_fields_t::REC_TYPE_CONTENT; 
			flds.considerCount = true;
            break;
        case '1':  {
            flds.weight = WEIGHT_BOOST(NAME); 
            if( loader.d_loader.d_title_avgLength ) {
                size_t titleLength = docTitle.length();
                if( !titleLength ) {
                    uint32_t docId = loader.d_loader.getDocIdByName( flds.name ) ;
                    if( docId != 0xffffffff ) {
                        titleLength = loader.d_loader.getDocTitleLength(docId);
                    }
                }
                if( titleLength ) {
                    double toAvg = (double)( titleLength ) / (double) ( loader.d_loader.d_title_avgLength );
                    // adjFactor = std::sqrt( 2.0/( 1.0+ std::sqrt(toAvg) ) );
                    flds.weight*= adjFactor;
                }
            }
            flds.recType = phrase_fields_t::REC_TYPE_TITLE; 
			flds.considerCount = false;
            break;
        }
        case '2': 
            flds.weight = WEIGHT_BOOST(KEYWORD); 
            flds.recType = phrase_fields_t::REC_TYPE_KEYWORD; 
			flds.considerCount = true;
            break;
        case '4': 
            flds.weight = WEIGHT_BOOST(RUBRIC); 
            flds.recType = phrase_fields_t::REC_TYPE_RUBRIC; 
			flds.considerCount = true;
            break;
        }
    } 

    /// phrase number
    tok = ( pipe < buf_end ? pipe +1 : 0 );
    pipe = ( tok < buf_end ? strchr( tok, '|' ) : 0 );
    if( !pipe ) return 1;
    *pipe= 0;
    flds.phraseNum = atoi(tok);

    if( flds.phraseNum == 0 && flds.recType == phrase_fields_t::REC_TYPE_TITLE ) {
        flds.weight+=( adjFactor* WEIGHT_BOOST(FIRST_PHRASE) );
    }

    /// phrase text
    tok = ( pipe < buf_end ? pipe +1 : 0 );

    flds.text = tok;
    
    return 0;
}
}

void ZurchPhrase_DocLoader::readDocContentFromFile( const char* fn, size_t maxLineLen )
{
    FILE* fp = fopen( fn, "r" );
    if( !fp ) {
        std::cerr << "Phrase Loader cant open " << fn << std::endl;
        return;
    }
    std::vector<char> bufVec;
    bufVec.resize( maxLineLen );

    char* buf = &(bufVec[0]);
    phrase_fields_t flds;
    std::string docName;
    uint32_t docId = 0xffffffff; 
    size_t totalSz = 0, docCount = 0;
    std::cerr << "reading document contents" ;
    ay::stopwatch timer;

    std::string docContentStr;
    docContentStr.reserve( maxLineLen );
    
    #define BUF_BEGINSWITH( x ) !strncmp( x, buf, sizeof(x)-1 ) 
    std::string docIdStr;
    size_t lineNum = 0;
    while( fgets(buf, bufVec.size()-1, fp ) ) {
        ++lineNum;
        bufVec.back()=0;
        size_t buf_sz = strlen(buf);
        if( buf_sz ) --buf_sz;
        buf[ buf_sz ] = 0;
        
        if( BUF_BEGINSWITH("C_BEGIN|")) { // C_BEGIN|XXX.YYY
            const char* n = buf+sizeof("C_BEGIN|")-1;
            docIdStr.assign( n );
            docId = d_loader.addDocName( docIdStr.c_str() );
        } else
        if( BUF_BEGINSWITH("C_END|")) { // C_END|XXX.YYY
            if( docId != 0xffffffff ) {
                d_loader.addDocContents( docId, docContentStr.c_str() );
            }
            ++docCount;

            docIdStr.clear();
            docContentStr.clear();
            docId = 0xffffffff;
            totalSz += docContentStr.length();
        } else 
        if( BUF_BEGINSWITH("TITLE|")) { // TITLE|XXX.YYY|Title text
            char* n = buf+sizeof("TITLE|")-1;
            char* pipe = strchr( n, '|' ); 
            if( pipe ) {
                *pipe =0;
                docId = d_loader.addDocName( n );
                const char* title = pipe+1;
                d_loader.setDocTitle( docId, title );

                int module = atoi( n );
                d_loader.index().d_docDataIdx.simpleIdx().Int.append( "module", docId, module );
            } else {
                std::cerr << "Zurch Doc Contents Reader Error, Malformed TITLE  at line " <<  lineNum << std::endl;
            }
        } else if( BUF_BEGINSWITH("POP.WEIGHT|")) { /// popularity weight
            char* n = buf+sizeof("POP.WEIGHT|")-1;
            char* pipe = strchr( n, '|' ); 
            if( pipe ) {
                *pipe=0;

				docIdStr.assign( n );
				docId = d_loader.addDocName( docIdStr.c_str() );

                const char* wStr = pipe+1;
                int32_t weight = atoi( wStr );
                d_loader.index().setExtWeight(docId, weight );
            } else {
                std::cerr << "Zurch Doc Contents Reader Error. Malformed POP.WEIGHT at line " <<  lineNum << std::endl;
            }
        } else { /// regular text - must be content
            if( docId != 0xffffffff ) { /// only if a document is currently open
                docContentStr.append( buf );
            }
        }
        if( ! ((docCount)%1000 ) ) 
            std::cerr << ".";
    }
    std::cerr << " done. " << docCount << 
    " docs totaling " << 
    ((double)totalSz)/1000000.0 << 
    "MB in " << timer.calcTime() << " seconds" << std::endl;
}
void ZurchPhrase_DocLoader::readPhrasesFromFile( const char* fn, bool noSort )
{
    FILE* fp = fopen( fn, "r" );
    if( !fp ) {
        std::cerr << "Phrase Loader cant open " << fn << std::endl;
        return;
    }
    ay::stopwatch localTimer;
    std::cerr << "Loading zurch doc phrases from " << fn << std::endl;
    const size_t BUF_SZ = 64*1024;
    char buf[ BUF_SZ ];
    phrase_fields_t flds;
    std::string docName;
    std::string text;
    uint32_t docId = 0xffffffff; 
    size_t numPhrase = 0;
    d_loader.parserSetup();
    std::string docTitle;
    
    while( fgets(buf, sizeof(buf)-1, fp ) ) {
        buf[ sizeof(buf)-1 ] = 0;
        size_t buf_sz = strlen(buf);
        if( buf_sz ) --buf_sz;
        buf[ buf_sz ] = 0;
        phrase_fmt_get_fields( *this, flds, buf, buf_sz, docTitle );
        
        if( flds.name && flds.text ) {
            if( docName != flds.name ) {
                docName.assign( flds.name );
                docId = d_loader.addDocName( docName.c_str() );
                docTitle = d_loader.getDocTitle(docId);
            }
            
            d_loader.setCurrentWeight(flds.weight);
            bool reuseBarz = (text == flds.text);
            if( !reuseBarz ) 
                text = flds.text;

			d_loader.index().setConsiderFeatureCount(flds.considerCount);
            d_loader.addDocFromString( docId, text + " ", d_loadStats, reuseBarz );
            
            ++numPhrase;
            if( !(numPhrase%10000) ) {
                std::cerr << ".";
            }
        }
    }
    fclose(fp);
    if( !noSort )
        d_loader.index().d_docDataIdx.simpleIdx().sort();
    std::cerr << numPhrase<<     " phrases done in " << localTimer.calcTime() << " seconds\n";
}


void ZurchLongXMLParser::readFromFile( const char* fname )
{
    ay::stopwatch timer;
    std::cerr << "zurch indexing data in " << fname << " ";
    std::ifstream fp;
    fp.open( fname );
    if( fp.is_open() ) {
        ZurchLongXMLParser_anon prs(*this);
        prs.init().parse(fp);
        std::cerr << prs.parser.d_numCallbacks << " documents loaded in " << timer.calcTime() << " seconds " << std::endl;
    } else {
        ay::print_absolute_file_path( (std::cerr << "ERROR: ZurchLongXMLParser cant open file \"" ), fname ) << "\"\n";
    }
    loadingDoneCallback();
}

} // namespace zurch 
