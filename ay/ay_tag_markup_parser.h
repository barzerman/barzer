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
    std::vector< char > d_buf; /// always contains txt between the tags 
    size_t d_bufOffset, d_readOffset;
    bool d_endReached;
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
    xhtml_parser_state( std::istream& fp, size_t bufSz = DEFAULT_BUF_SZ ) : 
        d_stream(fp), 
        d_buf(bufSz), 
        d_bufOffset(0), d_readOffset(0),
        d_endReached(false),
        d_cbReason(CB_UNKNOWN)
        { d_buf.back()=0; }
};

/// CB( const xhtml_parser_state&, const char* s`, size_t sz )
template <typename CB>
class xhtml_parser : public xhtml_parser_state {
public:
    CB& d_cbTagOpen, d_cbTagClose, d_cbTxt; 

    xhtml_parser_state& state(int cbReason) { return ( d_cbReason=cbReason, *(static_cast<xhtml_parser*>(this))); }
    const xhtml_parser_state& state() const { return *(static_cast<xhtml_parser*>(this)); }

    xhtml_parser( std::istream& fp, CB& oCB, CB& cCB, CB& txtCB, size_t bufSz = xhtml_parser_state::DEFAULT_BUF_SZ ) : 
        xhtml_parser_state(fp,bufSz ) ,
        d_cbTagOpen(oCB),
        d_cbTagClose(cCB),
        d_cbTxt(txtCB)
    {}

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
    void drainBuffer( )
    {
        /// buffer to drain starts at d_bufOffset
        std::string tag;
        while( d_bufOffset< d_buf.size() ) {
            // finding next tag
            const char* buf_start = &(d_buf[0]);
            const char* buf_offset = buf_start + d_bufOffset;
            const char* lt = strchr( buf_offset, '<' );
            const char* gt = ( lt ? strchr( lt, '>' ) : 0 );
            if( gt ) { /// complete tag has been detected
                if( textIsGood(buf_offset, lt) )
                    d_cbTxt(  state(CB_TEXT), buf_offset, lt-buf_offset ); 
                if( lt[1] == '/' ) {  /// this is a closing tag 
                    if( d_tagStack.size() ) 
                        d_tagStack.pop_back( );
                    d_cbTagClose( state(CB_TAG_CLOSE), lt, (gt-lt)+1 );
                } else {              /// this is an opening tag
                    if( gt>lt && *(gt-1) == '/' ) { /// this is a complete tag <XXX/> 
                        d_tagStack.push_back( std::string(lt, (gt-lt)+1 ) );
                        d_cbTagOpen( state(CB_TAG_OPEN), lt, (gt-lt)+1 );
                        d_cbTagClose( state(CB_TAG_CLOSE), lt, (gt-lt)+1 );
                        d_tagStack.pop_back( );
                    } else { /// this is just a regular opening tag
                        d_tagStack.push_back( std::string(lt, (gt-lt)+1 ) );
                        d_cbTagOpen( state(CB_TAG_OPEN), lt, (gt-lt)+1 );
                    }
                }
                d_bufOffset = 1+(gt-buf_start);
            } else if( lt ) { // next tag opening detected but it's an incomplete tag
                if( textIsGood(buf_offset, lt) )
                    d_cbTxt( state(CB_TEXT), buf_offset, lt-buf_offset ); 
                size_t old_sz = d_buf.size()- (lt-buf_start);
                if( lt> buf_start ) {
                    memmove( &(d_buf[0]), lt, old_sz );
                    lt=buf_start;
                    d_readOffset= old_sz;
                    size_t bytesToRead = d_buf.size()-d_readOffset, bytesRead = 0;
                    d_stream.read( &(d_buf[0])+ d_readOffset, bytesToRead );
                    if( (bytesRead=d_stream.gcount())< bytesToRead ) 
                        d_endReached= true;
                    if( !bytesRead )
                        return;
                    d_readOffset=0;
                    gt = strchr(lt,'>');
                    if( gt ) { // read more stuff from disk and now the tag is closing
                        if( gt>lt && *(gt-1) == '/' ) { // complete tag
                            d_tagStack.push_back( std::string(lt, (gt-lt)+1 ) );
                            d_cbTagOpen( state(CB_TAG_OPEN), lt, (gt-lt)+1 );
                            d_cbTagClose( state(CB_TAG_CLOSE), lt, (gt-lt)+1 );
                            d_tagStack.pop_back( );
                        }  else if(lt[1] =='/') { // closing tag
                            d_cbTagClose( state(CB_TAG_CLOSE), lt, (gt-lt)+1 );
                            if( d_tagStack.size() ) 
                                d_tagStack.pop_back( );
                        } else { // opening tag
                            d_cbTagOpen( state(CB_TAG_OPEN), lt, (gt-lt)+1 );
                        }
                    } else if(d_endReached) { // even after reading more stuff from disk the tag isnt closing end has been reached
                        break;
                    } else 
                       ++d_bufOffset;
                }
            } else if( !d_endReached ) { // we havent reached a tag at all yet theres soemthing else left in the stream 
                if( d_bufOffset > 0 ) { // we'll try to move filled buffer back and then read the remaining piece
                    if( d_buf.size()> d_bufOffset ) {
                        size_t old_sz = d_buf.size()- d_bufOffset;
                        memmove( &(d_buf[0]), buf_offset, old_sz );
                        d_bufOffset=0;
                        d_readOffset=old_sz;
                    }
                } else { /// d_bufOffset is 0 so our entire buffer doesnt fit the stuff 
                    if( textIsGood(buf_offset, lt ) )
                        d_cbTxt( state(CB_TEXT), buf_offset, d_buf.size()-d_bufOffset );
                    d_readOffset= 0;
                    d_bufOffset=0;
                    return;
                }
            } else
                break;
        }
    }
    void parse()
    {
        d_endReached = false;
        size_t offs = 0, bytesRead = 0;
        
        do {
            size_t bytesToRead = d_buf.size()-d_readOffset;
            d_stream.read( &(d_buf[0]) + d_readOffset, bytesToRead );
            d_readOffset=0;
            if( (bytesRead=d_stream.gcount())< bytesToRead ) 
                d_endReached= true;
            drainBuffer();
        } while( !d_endReached );
    }
};

} // namespace ay 
