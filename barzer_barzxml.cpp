#include <barzer_barz.h>
#include <barzer_barzxml.h>
#include <barzer_universe.h>
#include <barzer_parse.h>
#include <barzer_server_request.h>
#include <barzer_server_response.h>
#include <limits>

namespace barzer {

namespace {

enum {
    TAG_BARZ, // top levek barz tag
    TAG_BEAD, // individual bead
    TAG_TOPICS, // topics (parent barz, contains entity)
    TAG_TOKEN, // token ... attribute (opt) stem
    TAG_FLUFF, // 
    TAG_ERC,   
    // date related beads
    TAG_DATE,   
    TAG_TIMESTAMP,   
    TAG_TIME,   

    TAG_LO,   // range low (parent must be range
    TAG_HI,   // range high (parent must be range
    TAG_PUNCT, // parent entlist, bead, erc 
    TAG_ENTITY, // parent entlist, bead, erc 
    TAG_ENTLIST, // entity list - parent barz
    TAG_NUM,    // number. parent - bead/lo/hi attribute t= (int|real)
    TAG_SRCTOK, // parent must be bead
    TAG_RANGE,  // parent is either bead or ERC . attribute order=(ASC|DESC) opt=(NOHI|NOLO|FULLRANGE)
    /// add new tags above this line
    TAG_INVALID, /// invalid tag
    TAG_MAX=TAG_INVALID 
};


/// OPEN handles 
#define DECL_TAGHANDLE(X) inline int xmlth_##X( BarzXMLParser& parser, int tagId, const char* tag, const char**attr, size_t attr_sz, bool open)

#define IS_PARENT_TAG(X) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X) )
#define IS_PARENT_TAG2(X,Y) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || parser.tagStack.back()== TAG_##Y) )
#define IS_PARENT_TAG3(X,Y,Z) ( parser.tagStack.size() && (parser.tagStack.back()== TAG_##X || \
    parser.tagStack.back()== TAG_##Y || parser.tagStack.back()== TAG_##Z) )

#define ALS_BEGIN    const char** attr_end = attr+ attr_sz;\
	for( const char** a=attr; a< attr_end; a+=2 ) {\
        const char* n = 0[ a ];\
        const char* v = 1[ a ]; \
        switch(n[1]) {

#define ALS_END }}

#define UNIVERSE parser.universe
#define GLOBALPOOLS parser.universe.getGlobalPools()

enum {
    TAGHANDLE_ERROR_OK,
    TAGHANDLE_ERROR_PARENT, // wrong parent 
    TAGHANDLE_ERROR_ENTITY, // misplaced entity 
};

DECL_TAGHANDLE(BARZ) { 
    if( open ) {
    } else { // closing BARZ - time to process
        QParser qparser(parser.universe);
        BarzStreamerXML response(parser.barz, parser.universe);
        qparser.barz_parse( parser.barz, parser.qparm );
        response.print(parser.reqParser.stream());
        parser.barz.clearWithTraceAndTopics();
    }
    return TAGHANDLE_ERROR_OK;
} 
DECL_TAGHANDLE(BEAD) { 
    if( !IS_PARENT_TAG(BARZ) ) 
        return TAGHANDLE_ERROR_PARENT;
    parser.barz.appendBlankBead();

    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(ENTITY) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG3(ENTLIST,BEAD,ERC) ) 
        return TAGHANDLE_ERROR_PARENT;

    BarzerEntity ent; 
    ALS_BEGIN
        case 's': ent.setSubclass(atoi(v)); break;
        case 'c': ent.setClass(atoi(v));    break;
        case 'i':{
            uint32_t strId = GLOBALPOOLS.internalString_getId(v);
            if( strId != 0xffffffff ) 
                ent.setId(strId);
            }break;
    ALS_END

    BarzerEntityRangeCombo* erc = parser.barz.getLastBead().get<BarzerEntityRangeCombo>();
    if( erc ) {
        erc->setEntity( ent );
    } else if( parser.isCurTag(TAG_BEAD)  ) {
        parser.barz.getLastBead().setAtomicData( ent );
    } else if( parser.isCurTag(TAG_ENTLIST) ) {
        BarzerEntityList* entList = parser.barz.getLastBead().get<BarzerEntityList>();
        if( entList ) 
            entList->addEntity( ent );
        else 
            return TAGHANDLE_ERROR_ENTITY;
    }
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(ERC) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;

    parser.barz.getLastBead().setAtomicData( BarzerEntityRangeCombo() );
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(FLUFF) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(HI) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;
    
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(LO) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG(RANGE) ) 
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(PUNCT) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG(BEAD) ) 
        return TAGHANDLE_ERROR_PARENT;

    parser.barz.getLastBead().setAtomicData( BarzerLiteral() );

    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(DATE) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG3(BEAD,LO,HI) ) 
        return TAGHANDLE_ERROR_PARENT;
     
    BarzerDate date;
    ALS_BEGIN
        case 'y': // y - year
            date.setYear( atoi(v) );
            break;
        case 'm': // m[on] - month
            date.setMonth( atoi(v) );
            break;
        case 'd': // d - day
            date.setDay( atoi(v) );
            break;
    ALS_END

    BarzerEntityRangeCombo* erc = parser.barz.getLastBead().get<BarzerEntityRangeCombo>();
    BarzerRange* range = ( erc ?  &(erc->getRange()) : parser.barz.getLastBead().get<BarzerRange>() );

    if( range ) {
        BarzerRange::Date* dr = range->get<BarzerRange::Date>() ;
        if( !dr ) {
            range->setData( BarzerRange::Date() );
            dr = range->get<BarzerRange::Date>() ;
        }
        if( parser.isCurTag(TAG_LO) ) {
            dr->first = date;
        } else if( parser.isCurTag(TAG_HI) ) {
            dr->second = date;
        }
    } else { // standalone date
        parser.barz.getLastBead().setAtomicData( date );
    }
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(TIMESTAMP) {
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG3(BEAD,LO,HI) ) 
        return TAGHANDLE_ERROR_PARENT;
    
     
    BarzerDateTime tstamp;
    int hh=0, mm=0, ss=0;
    ALS_BEGIN
        case 'y': // y - year
            tstamp.date.setYear( atoi(v) );
            break;
        case 'm': // m - month or minutes
            if( n[1] == 'o' )  // mo
                tstamp.date.setMonth( atoi(v) );
            else if ( n[1] == 'i' ) // mi
                mm = atoi(v);
            break;
        case 'd': // d - day
            tstamp.date.setDay( atoi(v) );
            break;
        case 'h': // d - day
            hh = atoi(v);
            break;
        case 's': // d - day
            ss = atoi(v);
            break;
    ALS_END

    if( hh||mm||ss ) 
        tstamp.timeOfDay.setHHMMSS( hh, mm, ss );

    BarzerEntityRangeCombo* erc = parser.barz.getLastBead().get<BarzerEntityRangeCombo>();
    BarzerRange* range = ( erc ?  &(erc->getRange()) : parser.barz.getLastBead().get<BarzerRange>() );

    if( range ) {
        BarzerRange::DateTime* dr = range->get<BarzerRange::DateTime>() ;
        if( !dr ) {
            range->setData( BarzerRange::DateTime() );
            dr = range->get<BarzerRange::DateTime>() ;
        }
        if( parser.isCurTag(TAG_LO) ) {
            dr->first = tstamp;
        } else if( parser.isCurTag(TAG_HI) ) {
            dr->second = tstamp;
        }
    } else { // standalone date
        parser.barz.getLastBead().setAtomicData( tstamp );
    }
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(TIME) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG3(BEAD,LO,HI) ) 
        return TAGHANDLE_ERROR_PARENT;
    
    int hh=0, mm=0, ss=0;
    ALS_BEGIN
        case 'm': // m - month or minutes
            mm = atoi(v);
            break;
        case 'h': // d - day
            hh = atoi(v);
            break;
        case 's': // d - day
            ss = atoi(v);
            break;
    ALS_END

    BarzerTimeOfDay theTime;
    if( hh||mm||ss ) 
        theTime.setHHMMSS( hh, mm, ss );

    BarzerEntityRangeCombo* erc = parser.barz.getLastBead().get<BarzerEntityRangeCombo>();
    BarzerRange* range = ( erc ?  &(erc->getRange()) : parser.barz.getLastBead().get<BarzerRange>() );

    if( range ) {
        BarzerRange::TimeOfDay* dr = range->get<BarzerRange::TimeOfDay>() ;
        if( !dr ) {
            range->setData( BarzerRange::TimeOfDay() );
            dr = range->get<BarzerRange::TimeOfDay>() ;
        }
        if( parser.isCurTag(TAG_LO) ) {
            dr->first = theTime;
        } else if( parser.isCurTag(TAG_HI) ) {
            dr->second = theTime;
        }
    } else { // standalone time
        parser.barz.getLastBead().setAtomicData( theTime );
    }
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(NUM) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG3(BEAD,LO,HI) ) 
        return TAGHANDLE_ERROR_PARENT;

    BarzerNumber num;
    ALS_BEGIN
        case 't': /// type 
            if( v[0] == 'i' )
                num.setInt();
            else 
                num.setReal();
            break;
    ALS_END

    BarzerEntityRangeCombo* erc = parser.barz.getLastBead().get<BarzerEntityRangeCombo>();
    BarzerRange* range = ( erc ?  &(erc->getRange()) : parser.barz.getLastBead().get<BarzerRange>() );

    if( range ) {
        if( num.isReal() ) {
            BarzerRange::Real* dr = range->get<BarzerRange::Real>() ;
            if( !dr ) {
                dr = range->set<BarzerRange::Real>();
                if( parser.isCurTag( TAG_LO ) ) {
                    dr->first = num;
                } else if( parser.isCurTag( TAG_HI ) ) {
                    dr->second = num;
                }
            }
        } else if(num.isInt()) {
            BarzerRange::Integer* dr = range->get<BarzerRange::Integer>() ;
            if( !dr ) {
                dr = range->set<BarzerRange::Integer>();
                if( parser.isCurTag( TAG_LO ) ) {
                    dr->first = num;
                } else if( parser.isCurTag( TAG_HI ) ) {
                    dr->second = num;
                }
            }
        }
    } else 
        parser.barz.getLastBead().setAtomicData( num );
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(RANGE) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG2(BEAD,ERC) )
        return TAGHANDLE_ERROR_PARENT;
    parser.barz.getLastBead().setAtomicData( BarzerRange() );
    return TAGHANDLE_ERROR_OK;
}
DECL_TAGHANDLE(SRCTOK) { 
    if( parser.barz.isListEmpty() || !IS_PARENT_TAG(BEAD) )
        return TAGHANDLE_ERROR_PARENT;
    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(TOPICS) { 
    return TAGHANDLE_ERROR_OK;
}

DECL_TAGHANDLE(TOKEN) { 
    uint32_t stemStrId = 0xffffffff;
     
    ALS_BEGIN
        case 's': {
            uint32_t strId = GLOBALPOOLS.string_getId(v);
            if( strId != 0xffffffff ) 
                parser.barz.getLastBead().setStemStringId( strId );
            }break; 
    ALS_END // ettr loop switch

    return TAGHANDLE_ERROR_OK;
}

#define SETTAG(X) (handler=((tagId=TAG_##X),xmlth_##X))
inline void tagRouter( BarzXMLParser& parser, const char* t, const char** attr, size_t attr_sz, bool open)
{
    char c0= toupper(t[0]),c1=(c0?toupper(t[1]):0),c2=(c1?toupper(t[2]):0),c3=(c2? toupper(t[3]):0);
    typedef int (*TAGHANDLE)( BarzXMLParser& , int , const char* , const char**, size_t, bool );
    TAGHANDLE handler = 0;
    int       tagId   =  TAG_INVALID;
    switch( c0 ) {
    case 'B': 
        if( c1== 'E' && c2 == 'A' )      // BEA ...
            SETTAG(BEAD);
        else if( c1== 'A' && c2 =='R') // BAR ...
            SETTAG(BARZ);
        break;
    case 'D':
        if( c1 == 'A' ) { // DA[te]
            SETTAG(DATE);
        }
        break;
    case 'E':
        if( c1== 'N' && c2 == 'T' )      // ENT
            SETTAG(BEAD);

        break;
    case 'H':
        break;
    case 'N':
        if( c1 == 'U' || !c1 ) 
            SETTAG(NUM);
        break;
    case 'T':
        if( !c1 || (c1 == 'O' && c2 == 'K') )
            SETTAG(TOKEN);
        if( c1 == 'I' && c2 =='M' && c3 == 'E' ) {
            if( t[4] == 'S' ) 
                SETTAG(TIMESTAMP);
            else 
                SETTAG(TIME);
        }
        break;
    }
    if( tagId != TAG_INVALID ) {
        if( handler ) {
            handler(parser,tagId,t,attr,attr_sz,open);
        }
        if( open ) {
            parser.tagStack.push_back( tagId );
        } else if( parser.tagStack.size()  ) {
            parser.tagStack.pop_back();
        } else { // closing tag mismatch
            // maybe we will report an error (however silent non reporting is safer) 
        }
    }
}



} // end of anonymous namespace 

void BarzXMLParser::takeTag( const char* tag, const char** attr, size_t attr_sz, bool open)
{
    tagRouter(*this,tag,attr,attr_sz,open);
}

void BarzXMLParser::setLiteral( BarzelBead& bead, const char* s, size_t s_len, bool isFluff )
{
    const BZSpell* spell = universe.getBZSpell();
    if( spell )  {
        uint32_t strId = 0xffffffff;
        
        bool isUserWord = false;
        BarzerLiteral* ltrl = bead.getLiteral();

        if( s[ s_len ] ) { // non null terminated string 
            std::string tmp(s,s_len);
            isUserWord = spell->isUsersWord( strId, tmp.c_str() );
        } else { // null terminated string
            isUserWord = spell->isUsersWord( strId, s );
        }
        if( isUserWord ) {
            if( isFluff ) {
                if( ltrl ) 
                    ltrl->setStop(strId);
                else
                    bead.setAtomicData( BarzerLiteral().setStop(strId) );
            } else {
                if( ltrl )
                    ltrl->setId( strId );
                else 
                    bead.setAtomicData( BarzerLiteral(strId) );
            }
        } else {
            bead.setAtomicData( BarzerString(s,s_len) );
        }
    }
}

void BarzXMLParser::takeCData( const char* dta, size_t dta_len )
{
    if( !tagStack.size() || barz.isListEmpty() ) {
        return;
    }
    BarzelBead& bead = barz.getLastBead();
    int curTag = tagStack.back();
    switch( curTag ) {
    case TAG_TOKEN:
        setLiteral( bead, dta, dta_len, false );
        break;
    case TAG_FLUFF:
        setLiteral( bead, dta, dta_len, true );
        break;
    case TAG_PUNCT:
        bead.setAtomicData( BarzerLiteral().setPunct(*dta) );
        break;
    case TAG_NUM:
        {
            BarzerNumber* num = bead.get<BarzerNumber>();
            if( num ) {
                if( dta[dta_len] ) {
                    std::string tmp;
                    tmp.assign( dta, dta_len );
                    num->set(tmp.c_str());
                } else 
                    num->set(dta);
            }
            bead.setAtomicData( *num );
        }
        break;
    default:
        // error can be reported here
        break;
    }
}

} // namespace barzer
