#pragma once
#include <stdint.h>
#include <ay_utf8.h>
#include <ctype.h>
#include <iostream>

namespace ay {

struct parse_token {
    const char* buf;
    size_t buf_sz;
    size_t offset; // offset in the original doc
    bool hasSeparators; /// will be set to true if potential separators are in the token
public:
    parse_token( const char* b) : buf(b), buf_sz(0), offset(0), hasSeparators(false) {}
    parse_token( const char* b, size_t b_sz, size_t of ) : buf(b), buf_sz(b_sz), offset(of), hasSeparators(false) {}

    void clear() { buf=0; buf_sz=0; offset=0; hasSeparators=false; }
    void set( const char* b, size_t sz, size_t o )
        { buf=b; buf_sz=sz; offset=o; hasSeparators=false; }
    bool empty() { return !buf; }
    void extend() { ++buf_sz; }
    bool isHasSeparators() const { return hasSeparators; }
    void setHasSeparators(bool v=true) { hasSeparators=v; }

    const char* getBuf() const { return buf; }
    size_t getBuf_sz() const { return buf_sz; }

    size_t getOffset() const { return offset; }
    void setOffset( size_t o) { offset=o; }
    
    bool isWhitespace() const { return ( buf && buf[0] == ' ' );}
    bool isNonPrintable() const { return ( buf && isascii(buf[0]) && !isprint(buf[0]) );}

    std::ostream& print( std::ostream& fp ) const 
    {
        if( isNonPrintable() ) {
            return fp << (hasSeparators?"SEP":"NOSEP") << "," << offset << "," << buf_sz << ":0x" << std::hex << static_cast<uint32_t>(buf[0]) ;
        } else {
            fp << (hasSeparators?"SEP":"NOSEP") << "," << offset << "," << buf_sz << ":\"" ;
            return fp.write(buf,buf_sz) << "\"";
        }
    }
};

inline std::ostream& operator <<( std::ostream& fp, const parse_token& t )
{
    if( t.getBuf() ) {
        return fp.write( t.getBuf(), t.getBuf_sz() );
    } else
        return fp << "";
}

class callback_tokenizer {
    /// .second in the pair is true if this separator char needs to be separated on
    /// in case it's false when such characters are encountered (potential separators) parse_token::hasSeparators 
    /// will be set to false
    typedef std::pair<CharUTF8,bool> SepChar;
    typedef std::vector<SepChar> SepCharVec;

    SepCharVec d_sep;
    bool        d_hasNonAsciiSeparators;  // default false
    SepCharVec& separator() { return d_sep; }
public:
    const SepCharVec& separator() const { return d_sep; }

    callback_tokenizer( const SepCharVec& v ) : d_sep(v), d_hasNonAsciiSeparators(false) {}
    callback_tokenizer( ) {}
    enum {
        IS_SEPARATOR_NO,
        IS_SEPARATOR_YES,
        IS_SEPARATOR_POTENTIAL,
    };

    inline int findCharSeparator( char c ) const
    {
        for( SepCharVec::const_iterator i = d_sep.begin(); i!= d_sep.end(); ++i ) {
            if( i->first.size() ==1 && c== i->first.getChar0() ) 
                return ( i->second ? IS_SEPARATOR_YES : IS_SEPARATOR_POTENTIAL );
        }
        return IS_SEPARATOR_NO;
    }
    int findUtf8Separator( const CharUTF8& c ) 
    {
        // 
        for( SepCharVec::const_iterator i = d_sep.begin(); i!= d_sep.end(); ++i ) {
            if( i->first == c ) 
                return ( i->second ? IS_SEPARATOR_YES: IS_SEPARATOR_POTENTIAL );
        }
        return IS_SEPARATOR_NO;
    }
    inline const char* findUtf8Separator( const char* s, const char* s_end, int& separatorStatus ) const
    {
        separatorStatus=IS_SEPARATOR_NO;
        if( !d_hasNonAsciiSeparators || s_end<=s ) {

            return ( (separatorStatus=IS_SEPARATOR_NO),nullptr);
        }
        char theS[5] = {0};
        size_t s_len=0;
        for( const char* tmp = s; tmp < s_end && !isascii(*tmp); ++tmp ) ++s_len;
        if( s_len <sizeof(theS) ) {
            switch(s_len) {
            case 4: theS[3]= s[3];
            case 3: theS[2]= s[2];
            case 2: theS[1]= s[1];
            case 1: theS[0]= s[0];
                break;
            default: return ( (separatorStatus=IS_SEPARATOR_NO),nullptr);
            }
        }
        for( SepCharVec::const_iterator i = d_sep.begin(); i!= d_sep.end(); ++i ) {
            if( i->first == theS ) {
                if( i->second ) {
                    return( (separatorStatus=IS_SEPARATOR_YES), s+(i->first.size()-1) );
                } else {
                    return ( (separatorStatus=IS_SEPARATOR_POTENTIAL),nullptr);
                }
            }
        }
        return 0;
    }

    void addSeparator( char c, bool isReal=true ) 
    {
        if( IS_SEPARATOR_NO == findCharSeparator(c) ) {
            d_sep.push_back( SepChar(CharUTF8(c),isReal) );
            if( !isascii(c) &&  !d_hasNonAsciiSeparators  )
                    d_hasNonAsciiSeparators= true;
        }
    }
    /// adds a single multibyte separator 
    void addSeparator( const char* sep, bool isReal=true ) 
    {
        if( isascii(*sep) )
            addSeparator( *sep, isReal );

        CharUTF8 uc(sep);
        if(findUtf8Separator(uc) == IS_SEPARATOR_NO ) {
            d_sep.push_back( SepChar(uc,isReal) );
        }
        if( !d_hasNonAsciiSeparators )
            d_hasNonAsciiSeparators=true;
    }
    void addSeparator( const StrUTF8& str, bool isReal=true ) 
    {
        for( auto i = str.begin(), i_end = str.end(); i!= i_end; ++i ) {
            d_sep.push_back( SepChar((*i),isReal) );
        }
    }
    /// converts s to utf8 and treats every glyph in the string as a separator 
    /// just a shorter form 
    void addSeparators( const char* s, bool isReal=true )
    {
        StrUTF8 us( s, strlen(s) );
        addSeparator( StrUTF8(s,strlen(s)), isReal );
    }

    /// adds all characters in the string as separators
    void addSingleCharSeparators( const char* sep, bool isReal =true) 
    {
        if( !sep ) 
            return;
        for( const char* s = sep; *s; ++s ) 
            addSeparator(*s,isReal);
    }
    

    /// CB must have bool operator()( const parse_token& )
    /// when callback returns false parsing will be terminated 
    template <typename CB>
    size_t tokenize( const char* str, size_t str_sz, CB& cb ) 
    {
        parse_token tok(str);
        size_t numTok = 1;
        for( const char* s = str, *str_end=str+str_sz; s!= str_end; ++s ) {
            if( tok.empty() ) 
                tok.set(s,0,(s-str)); // s-str ~ offset

            int separatorStatus= IS_SEPARATOR_NO;
            const char* endSeparator=0;
            if( isascii(*s) ) {
                separatorStatus =  findCharSeparator(*s);
            } else 
                endSeparator = findUtf8Separator(s, str_end, separatorStatus);
             
            if( separatorStatus == IS_SEPARATOR_YES ) {
                ++numTok;
                if( !tok.getBuf_sz() ) {
                    tok.set(s,(endSeparator? ((endSeparator-s)+1): 1),(s-str));
                    if( !cb(tok) )
                        return numTok;
                    tok.clear();
                } else {
                    if( !cb(tok) ) 
                        return numTok;
                    else {
                        tok.set(s,(endSeparator? ((endSeparator-s)+1): 1),(s-str));
                        if( !cb(tok) ) 
                            return numTok;
                        tok.clear();
                    }
                }
                if( endSeparator )
                    s=endSeparator;
            } else {
                if( separatorStatus == IS_SEPARATOR_POTENTIAL )
                    tok.setHasSeparators();
                tok.extend();
            }
        }
        if( !tok.empty() )
            cb(tok);
        return numTok;
    }
};


} // namespace ay
