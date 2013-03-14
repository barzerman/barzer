#include <zurch/zurch_loader_longxml.h>
#include <ay/ay_logger.h>
#include <ay/ay_util_time.h>
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

    typedef enum { 
        TXT_CONTENT=0,
        TXT_NAME=1,
        TXT_KEYWORDS=2
    } txt_type_t;
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
        parser.setMode(ay::xhtml_parser_state::MODE_HTML);
        parser.parse();
    }
};

} // anonymous namespace 

int ZurchLongXMLParser_Phraserizer::callback()
{
    PhraseizerCB pz( d_outFP, d_phraser );
    pz.d_docId = d_data.d_ModuleID + "." + d_data.d_ID;
    
    // NAME
    pz.parseText( d_data.d_DocName , PhraseizerCB::TXT_NAME,     " " );
    pz.parseText( d_data.d_Content , PhraseizerCB::TXT_CONTENT );
    pz.parseText( d_data.d_Keywords, PhraseizerCB::TXT_KEYWORDS, " " );
    return 0;
}
namespace {
    enum : DocFeatureLink::Weight_t {
        WEIGHT_BOOST_NONE=0, 
        WEIGHT_BOOST_NAME=1000,
        WEIGHT_BOOST_KEYWORD=2000
    };
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
        d_loader.setCurrentWeight(WEIGHT_BOOST_NAME);
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
    }
    { // KEYWORDS
        std::stringstream sstr;
        sstr << d_data.d_Keywords << " ";
        d_loader.setCurrentWeight(WEIGHT_BOOST_KEYWORD);
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
    }
    { /// CONTENT
        std::stringstream sstr;
        sstr << d_data.d_Content;
        d_loader.setCurrentWeight(WEIGHT_BOOST_NONE);
        d_loader.addDocFromStream( docId, sstr, d_loadStats );
        if( d_loader.hasContent() ) 
            d_loader.addDocContents( docId, d_data.d_Content );
    }
    return 0;
}

namespace {

struct phrase_fields_t {
    DocFeatureLink::Weight_t weight;
    char* name;
    char* text;

    phrase_fields_t(): weight(WEIGHT_BOOST_NONE), name(0), text(0) {}
    void clear() 
    {
        weight = WEIGHT_BOOST_NONE;
        name = text = 0;
    }
}; 


int phrase_fmt_get_fields( phrase_fields_t& flds, char* buf, size_t buf_sz )
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
    if( !pipe ) return 1;
    if( tok[0] =='T' ) {
        switch( tok[1] ) {
        case '0': flds.weight = WEIGHT_BOOST_NONE; break;
        case '1': flds.weight = WEIGHT_BOOST_NAME; break;
        case '2': flds.weight = WEIGHT_BOOST_KEYWORD; break;
        }
    } 

    /// phrase number
    tok = ( pipe < buf_end ? pipe +1 : 0 );
    pipe = ( tok < buf_end ? strchr( tok, '|' ) : 0 );
    if( !pipe ) return 1;

    /// phrase text
    tok = ( pipe < buf_end ? pipe +1 : 0 );

    flds.text = tok;
    
    return 0;
}
}

void ZurchPhrase_DocLoader::readDocContentFromFile( const char* fn )
{
    FILE* fp = fopen( fn, "r" );
    if( !fp ) {
        std::cerr << "Phrase Loader cant open " << fn << std::endl;
        return;
    }
    const size_t BUF_SZ = 16*1024*1024;
    char buf[ BUF_SZ ];
    phrase_fields_t flds;
    std::string docName;
    uint32_t docId = 0xffffffff; 
    size_t totalSz = 0, docCount = 0;
    std::cerr << "reading document contents" ;
    ay::stopwatch timer;
    while( fgets(buf, sizeof(buf)-1, fp ) ) {
        buf[ sizeof(buf)-1 ] = 0;
        size_t buf_sz = strlen(buf);
        buf[ buf_sz ] = 0;
        if( buf_sz ) --buf_sz;
        
        char* pipe = strchr( buf, '|' );
        if( pipe && pipe< buf+buf_sz) 
            *pipe = 0;
        else
            continue;
        docId = d_loader.addDocName( buf );
        d_loader.addDocContents( docId, pipe+1 );
        if( ! ((++docCount)%1000 ) ) 
            std::cerr << ".";
        totalSz += buf_sz- strlen(pipe);
    }
    std::cerr << " done. " << docCount << 
    " docs totaling " << 
    ((double)totalSz)/1000000.0 << 
    "MB in " << timer.calcTime() << " seconds" << std::endl;
}
void ZurchPhrase_DocLoader::readFromFile( const char* fn )
{
    FILE* fp = fopen( fn, "r" );
    if( !fp ) {
        std::cerr << "Phrase Loader cant open " << fn << std::endl;
        return;
    }
    const size_t BUF_SZ = 64*1024;
    char buf[ BUF_SZ ];
    phrase_fields_t flds;
    std::string docName;
    uint32_t docId = 0xffffffff; 
    while( fgets(buf, sizeof(buf)-1, fp ) ) {
        buf[ sizeof(buf)-1 ] = 0;
        size_t buf_sz = strlen(buf);
        buf[ buf_sz ] = 0;
        if( buf_sz ) --buf_sz;
        phrase_fmt_get_fields( flds, buf, buf_sz );
        
        if( flds.name && flds.text ) {
            if( docName != flds.name ) {
                docName.assign( flds.name );
                docId = d_loader.addDocName( docName.c_str() );
            }

            if( flds.weight == WEIGHT_BOOST_NAME ) 
                d_loader.setDocTitle( docId, flds.name );

            d_loader.setCurrentWeight(flds.weight);
            d_loader.addDocFromString( docId, flds.text, d_loadStats );
        }
    }
    fclose(fp);
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
}

} // namespace zurch 
