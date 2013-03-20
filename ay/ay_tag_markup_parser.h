
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <vector>
#include <string>
#include <iostream>

/// this parses generic markup 
namespace ay {

/// parses generic tag markups (XML/HTML) 
/// doesnt validate anything ... invokes callbacks on tag opening and closing 
/// maintains a stack of tags ... closing tag ALWAYS pops from the stack (even if everything is completely malformed)
/// also invokes callback for the text between tags 
/// assumes the following format
/// <XXX> TEXT TEXT </XXX> <YYY/> 
struct xhtml_parser_state {
    std::istream& d_stream;

    std::vector<std::string> d_tagStack;
    
    std::string getTag( size_t i ) const 
    {
        if( i< d_tagStack.size() ) {
            const char* tbeg = d_tagStack[i].c_str()+1;
            if( *tbeg == '/' ) ++tbeg;
            const char* tend = tbeg;
            for( ;isalnum(*tend); ++tend ) ;

            return ( tend> tbeg ? std::string( tbeg, tend-tbeg ): std::string() );
        } else
            return std::string();
    }

    std::vector< char > d_buf; /// always contains txt between the tags 
    size_t d_bufOffset, d_readOffset;
    bool d_endReached;
    size_t d_numTagsSeen;
    enum : size_t {
        DEFAULT_BUF_SZ = 256*1024
    };
    // reason for callback
    enum {
        CB_UNKNOWN,
        CB_TAG_OPEN,
        CB_TAG_CLOSE,
        CB_TEXT
    };
    int d_cbReason;

    typedef enum {
        MODE_HTML,
        MODE_XHTML
    } xhtml_mode_t;
    xhtml_mode_t d_mode; // MODE_HTML 

    xhtml_parser_state& setMode( xhtml_mode_t m) { return (d_mode = m,*this); }
    void setModeHtml() { d_mode = MODE_HTML; }
    bool isModeHtml() const { return d_mode == MODE_HTML; }
    void setModeXhtml() { d_mode = MODE_XHTML; }
    bool isModeXhtml() const { return d_mode == MODE_XHTML; }
    
    bool isCallbackText( ) const { return d_cbReason ==CB_TEXT ; }
    inline static bool isHtmlTagAlwaysComplete( const char* lt, const char* gt ) 
    {
        while( *lt == '<' || *lt == '/' ) ++lt ; 
        if( gt<= lt ) return true;
        size_t sz = gt-lt; 

        size_t len = 0;
        char c0 = ( isalnum(*lt) ? (len=1,*lt): 0 ) , 
            c1 = (c0 && isalpha(lt[1]) ? (len=2,lt[1]) : 0),
            c2 = (c1 && isalpha(lt[2]) ? (len=3,lt[2]) : 0);

        if( c2 && isalnum(lt[3]) ) 
            len = 4; // at least 4

        if( len == 2 ) { // 2 letter tags 
            if( c1 == 'r' || c1=='R' ) // bR hR
                return (c0 == 'b' || c0=='B' || c0 =='h' || c0=='H');
            else if( c1 == 'l' || c1=='L' )  // hl
                return (c0 =='h' || c0=='H');
            else 
                return false;
        }  else if( len == 3 ) {
            if( !strncasecmp( lt, "img", 3) )
                return true;
        }
        return false;
    }
    xhtml_parser_state( std::istream& fp, size_t bufSz = DEFAULT_BUF_SZ ) : 
        d_stream(fp), 
        d_buf(bufSz), 
        d_bufOffset(0), d_readOffset(0),
        d_endReached(false),
        d_numTagsSeen(0),
        d_cbReason(CB_UNKNOWN),
        d_mode(MODE_HTML)
    { d_buf.back()=0; }
};

/// CB( const xhtml_parser_state&, const char* s`, size_t sz )
template <typename CB>
class xhtml_parser : public xhtml_parser_state {
public:

    CB& d_cbTagOpen, &d_cbTagClose, &d_cbTxt; 

    xhtml_parser_state& state(int cbReason) { return ( d_cbReason=cbReason, *(static_cast<xhtml_parser*>(this))); }
    const xhtml_parser_state& state() const { return *(static_cast<xhtml_parser*>(this)); }

    xhtml_parser( std::istream& fp, CB& oCB, CB& cCB, CB& txtCB, size_t bufSz = xhtml_parser_state::DEFAULT_BUF_SZ ) : 
        xhtml_parser_state(fp,bufSz ) ,
        d_cbTagOpen(oCB),
        d_cbTagClose(cCB),
        d_cbTxt(txtCB)
    {}

    xhtml_parser( CB& ocb, CB& ccb, CB& tcb, const xhtml_parser_state& state ) :
        xhtml_parser_state(state),
        d_cbTagOpen(ocb),
        d_cbTagClose(ccb),
        d_cbTxt(tcb)
    { }

    xhtml_parser( CB& cb, const xhtml_parser_state& state ) :
        xhtml_parser_state(state),
        d_cbTagOpen(cb),
        d_cbTagClose(cb),
        d_cbTxt(cb)
    { }

    xhtml_parser( std::istream& fp, CB& cb, size_t bufSz = xhtml_parser_state::DEFAULT_BUF_SZ ) : 
        xhtml_parser_state(fp,bufSz ) ,
        d_cbTagOpen(cb),
        d_cbTagClose(cb),
        d_cbTxt(cb)
    {}

    bool hasNonspace( const char* s, size_t len  )  const
    {
        for( const char* s_end = s+len; s< s_end; ++s ) 
            if( !isspace(*s) ) 
                return true;

        return false;
    }
    bool textIsGood( const char* beg, const char* end ) const {
        return( end > beg && *beg != '<' && *beg != '>' );
    }
     
    bool isCompleteTag( const char* lt, const char* gt ) const
    {
        if( gt> lt )  {
            return( *(gt-1) =='/' || (isModeHtml() && isHtmlTagAlwaysComplete(lt,gt) ) );
        } else
            return false;
    }
    void drainBuffer( )
    {
        /// buffer to drain starts at d_bufOffset
        std::string tag;
        
        while( d_bufOffset< d_buf.size()-1 ) {
            // finding next tag
            const char* buf_start = &(d_buf[0]);
            const char* buf_offset = buf_start + d_bufOffset;
            const char* lt = strchr( buf_offset, '<' );
            const char* gt = ( lt ? strchr( lt, '>' ) : 0 );
            if( gt ) { /// complete tag has been detected
                ++d_numTagsSeen;
                if( textIsGood(buf_offset, lt) )
                    d_cbTxt(  state(CB_TEXT), buf_offset, lt-buf_offset ); 
                if( lt[1] == '/' ) {  /// this is a closing tag 
                    if( d_tagStack.size() ) 
                        d_tagStack.pop_back( );
                    d_cbTagClose( state(CB_TAG_CLOSE), lt, (gt-lt)+1 );
                } else {              /// this is an opening tag
                    const size_t xsz = (gt-lt)+1 ;
                    if( isCompleteTag(lt,gt) ) { /// this is a complete tag <XXX/> 
                        d_tagStack.push_back( std::string(lt, xsz ) );
                        d_cbTagOpen( state(CB_TAG_OPEN), lt, xsz );
                        d_cbTagClose( state(CB_TAG_CLOSE), lt, xsz );
                        d_tagStack.pop_back( );
                    } else { /// this is just a regular opening tag
                        d_tagStack.push_back( std::string(lt, xsz) );
                        d_cbTagOpen( state(CB_TAG_OPEN), lt, xsz );
                    }
                }
                d_bufOffset = 1+(gt-buf_start);
            } else if( lt ) { // next tag opening detected but it's an incomplete tag
                if( textIsGood(buf_offset, lt) )
                    d_cbTxt( state(CB_TEXT), buf_offset, lt-buf_offset ); 
                size_t old_sz = d_buf.size()-1- (lt-buf_start);
                if( lt> buf_start ) {
                    memmove( &(d_buf[0]), lt, old_sz );
                    lt=buf_start;
                    d_readOffset= old_sz;
                    size_t bytesToRead = d_buf.size()-1-d_readOffset, bytesRead = 0;
                    d_stream.read( &(d_buf[0])+ d_readOffset, bytesToRead );
                    if( (bytesRead=d_stream.gcount())< bytesToRead ) 
                        d_endReached= true;
                    if( !bytesRead )
                        return;
                    d_readOffset=0;
                    gt = strchr(lt,'>');
                    if( gt ) { // read more stuff from disk and now the tag is closing
                        const size_t xsz = (gt-lt)+1 ;
                        if( isCompleteTag(lt,gt) ) { // complete tag
                            d_tagStack.push_back( std::string(lt, xsz) );
                            d_cbTagOpen( state(CB_TAG_OPEN), lt, xsz );
                            d_cbTagClose( state(CB_TAG_CLOSE), lt, xsz );
                            d_tagStack.pop_back( );
                        }  else if(lt[1] =='/') { // closing tag
                            d_cbTagClose( state(CB_TAG_CLOSE), lt, xsz );
                            if( d_tagStack.size() ) 
                                d_tagStack.pop_back( );
                        } else { // opening tag
                            d_cbTagOpen( state(CB_TAG_OPEN), lt, xsz );
                        }
                    } else if(d_endReached) { // even after reading more stuff from disk the tag isnt closing end has been reached
                        break;
                    } else 
                       ++d_bufOffset;
                }
            } else if( !d_endReached ) { // we havent reached a tag at all yet theres soemthing else left in the stream 
                d_numTagsSeen=0;
                if( d_bufOffset > 0 ) { // we'll try to move filled buffer back and then read the remaining piece
                    if( d_buf.size()-1> d_bufOffset ) {
                        size_t old_sz = d_buf.size()-1- d_bufOffset;
                        memmove( &(d_buf[0]), buf_offset, old_sz );
                        d_bufOffset=0;
                        d_readOffset=old_sz;
                        return;
                    }
                } else { /// d_bufOffset is 0 so either our entire buffer doesnt fit the stuff 
                    if( textIsGood(buf_offset, lt ) )
                        d_cbTxt( state(CB_TEXT), buf_offset, d_buf.size()-1-d_bufOffset );
                    d_readOffset= 0;
                    d_bufOffset=0;
                    return;
                }
            } else if( !d_numTagsSeen ) {
                if( d_bufOffset < d_buf.size()-1 ) {
                    size_t theLen = strlen(buf_offset);
                    if( theLen > 0 )
                    d_cbTxt( state(CB_TEXT), buf_offset, theLen );
                }
                d_readOffset= 0;
                d_bufOffset=0;
                return;
            } else
                break;
        }
    }
    void parse()
    {
        d_endReached = false;
        size_t offs = 0, bytesRead = 0;
        d_numTagsSeen = 0; 
        do {
            if( d_readOffset+1>= d_buf.size() )
                break;
            size_t bytesToRead = d_buf.size()-1-d_readOffset;
            d_stream.read( &(d_buf[0]) + d_readOffset, bytesToRead );
            d_buf.back()=0;
            d_readOffset=0;
            if( (bytesRead=d_stream.gcount())< bytesToRead ) 
                d_endReached= true;
            drainBuffer();
        } while( !d_endReached );
    }
};

} // namespace ay 
