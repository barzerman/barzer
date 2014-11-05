#pragma once

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <ay_parse.h>

namespace ay {

namespace streamer {

class bin_file {
    protected:
    FILE* fp;
    bin_file( FILE* f) : fp(f) { }
    ~bin_file() { if( fp && fp != stdout && fp != stdin ) fclose(fp); }
public:
    bool valid() const { return fp; }
    bool is( FILE* f ) const { return fp == f; }
};

/// these are superlight FILE* raii objects . they use fread/fwrite directly
/// please be careful with operator()( T ) - T MUST BE A BCS TYPE - integral types ONLY !!!!
struct bin_file_writer : public bin_file {
    bin_file_writer( const char* fname=0 ) :
        bin_file( (fname && *fname) ? fopen( fname, "wb" ): stdout )
        { if( !fp ) fprintf( stderr, "cant open file %s for writing\n", fname ); }

    size_t operator()( const void * ptr, size_t size, size_t count ) { return( fp? size*fwrite( ptr, size, count, fp ): 0 ); }
    size_t operator()( const void * ptr, size_t size) { return (*this)( ptr, size,1); }
    template <typename T> size_t operator()( T x ) { return (*this)(&x,sizeof(x)); }

    template <typename T> size_t write( T x ) { return (*this)( &x, sizeof(T)); }
    // updates the value starting at byte 0 of the file
    template <typename T> void update_at_file_start( const T x )
    {
        if( fp  ) {
            fseek( fp, 0, SEEK_SET );
            write<T>( x );
        }
    }
};

struct bin_file_reader : public bin_file {
    bin_file_reader( const char* fname=0 ) :
        bin_file( (fname && *fname) ? fopen( fname, "rb" ): stdout )
        { if( !fp ) fprintf( stderr, "cant open file %s for reading\n", fname ); }

    size_t operator()( void * ptr, size_t size, size_t count ) { return( fp ?  size*fread( ptr, size, count, fp ): 0 ); }
    size_t operator()( void * ptr, size_t size) { return (*this)(ptr, size,1); }

    template <typename T> size_t operator()( T& x ) { return (*this)(&x,sizeof(x)); }

    template <typename T> size_t read(T& x) { return (*this)(&x,sizeof(T)); }
    template <typename T> T read() {
        T v;
        return ( (*this)(v) ? v: T(0) );
    }
};

template <typename T>
size_t serialize( bin_file_writer& writer, const std::vector<T>& vec )   {
    size_t bytes = writer.write<uint32_t>( (uint32_t)(vec.size()) );
    if( vec.size() ) {
        bytes+= writer( &(vec[0]), sizeof(T), vec.size() );
    }
    return bytes;
}

template <typename T>
inline size_t deserialize( bin_file_reader& reader, std::vector<T>& vec ) {
    uint32_t vecSz = 0;
    size_t bytes = reader.read<uint32_t>(vecSz);
    if( vecSz ) {
        vec.resize( vecSz );
        bytes+=reader( &(vec[0]), sizeof(T), vec.size() );
    }
    return bytes;
}

} // streamer

/// iterates over lines in a file and invokes callback for each line
/// callback type CB must have int operator()( char* str, size_t str_len)
/// when callback returns a value different from 0 iteration is aborted
/// the callback can alter str
/////
struct CountLineCB {
    size_t lines;
    CountLineCB(): lines(0) {}
    int operator()( char* str, size_t str_len) { return ( ++lines, 0 ); }
};

class FgetsGenerator {
        FILE* d_fp;
        size_t d_bufSize;

        class Buf {
        public:
            char* buf;
            Buf( size_t sz ) { buf = new char[ sz ]; }
            ~Buf( ) { delete buf; }
        private:
            Buf( const Buf& ) {}
        };
    public:
        enum { ERR_NONE=0, ERR_LINE_TOO_LONG = 1 } d_err;

        enum  { DEFAULT_BUF_SZ = 16*1024*1024 };


        FgetsGenerator( FILE* fp, size_t bufSize=DEFAULT_BUF_SZ ) :
            d_fp(fp),
            d_bufSize(bufSize),
            d_err(ERR_NONE)
        {}
        //
        /// when numIterations == 0 will iterate until the file is exhausted
        template <class CB>
        void generate( CB& cb, size_t numIterations = 0 ) {
            if( !d_fp )
                return;
            Buf b( d_bufSize );
            int cbRc = 0;
            size_t bufOffset = 0;
            if( !numIterations ) {
                numIterations = SIZE_MAX;
            }
            size_t line = 0;
            for( ; !cbRc;  ) {
                char* bufDest =  b.buf+bufOffset;
                size_t bufSz = ( d_bufSize > bufOffset+1 ? d_bufSize-1-bufOffset: 0 );

                if( !bufSz ) {
                    d_err = ERR_LINE_TOO_LONG;
                    return;
                }

                if( size_t bytes = fread(bufDest, 1, bufSz, d_fp) ) {
                    // draining buffer
                    b.buf[ d_bufSize -1 ] =0;
                    for( char* s = b.buf; s; ) {
                        char* s_end = strchr( s, '\n' );
                        if( s_end ) {
                            *s_end = 0;
                            cbRc= cb( s, s_end-s);
                            if( numIterations <= line )
                                return;
                            ++line;
                            s=s_end +1;
                        } else {
                            if( s_end >s ) {
                                size_t mvSz = s_end-s;
                                memmove( b.buf, s, mvSz );
                                bufOffset = mvSz;
                            }
                            break;
                        }
                    }
                } else
                    return;
            }
        }
};

struct FileLineGenerator {
    FILE* d_fp;
    FgetsGenerator fgen;

    FileLineGenerator( const char* fileName =0) :
        d_fp( fileName ? fopen( fileName, "r" ) : stdin ),
        fgen( d_fp )
    {
        if( !d_fp ) {
            std::cerr << "ERROR: failed to open file \"" << fileName << "\" for reading\n";
        }
    }
    ~FileLineGenerator() { if( d_fp && d_fp != stdin ) fclose(d_fp); }
};

template <typename CB>
struct AsciiFileColumns {
    const CB& d_cb;
    size_t d_maxNumCol;
    size_t d_numLines;

    char d_sep;
    enum { DEFAULT_MAX_COL = 16 };
    std::vector<const char*> d_col;

    AsciiFileColumns& setSep( char sep ) { return (d_sep=sep, *this); }
    AsciiFileColumns& setMaxCol( size_t mx ) { return (d_maxNumCol=mx, *this); }

    AsciiFileColumns(const CB& cb, char sep ='|',size_t mc = DEFAULT_MAX_COL):
        d_cb(cb),
        d_maxNumCol(mc),
        d_numLines(0),
        d_sep(sep)
    {}
    int operator()( char* s, size_t s_len ) {
        ay::destructive_parse_separator(
            [&]( size_t tok_num, const char* tok) -> bool {
                if( tok_num> d_maxNumCol )
                    return true;
                d_col.push_back(tok);
                return 0;
            },
            s,
            d_sep
        );
        d_cb(d_col);
        d_col.clear();
        ++d_numLines;
        return 0;
    }
    void reset() { d_numLines = 0; }
    size_t parse_file( const char* fileName )
    {
        ay::FileLineGenerator g(fileName);
        g.fgen.generate(*this);
        return d_numLines;
    }
};

template <typename CB>
inline size_t parse_ascii_file(const char* fileName, char sep, const CB& cb)
{
    AsciiFileColumns<CB> parser(cb, sep);
    return parser.parse_file(fileName);
}

// this is a simplified invocation pair of macros
// use it for specifying code for processing individual lines
// AY_PAF_BEGIN(fileName,separator)
//     your code accessing vector<const char*> col  - vector of tokens
// AY_PAF_END
#define AY_PAF_BEGIN(fn,sep) ay::parse_ascii_file( fn, sep, [&](const std::vector<const char*>& col) -> int {
#define AY_PAF_END });

} // namespace ay ends
