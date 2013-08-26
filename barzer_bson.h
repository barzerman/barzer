#pragma once 

#include <string>
#include <vector>
#include <stdint.h>

namespace bson
{
class Encoder
{
    struct StackFrame
    {
        int32_t   size; // cumulative size
        size_t    sizeOffset;      // &buf[sizeOffset] - size of the document 

        StackFrame() : size(0), sizeOffset(0) {}
        StackFrame( int32_t sz, size_t szOffs) : size(sz), sizeOffset(szOffs) {}
    };

    // vector is faster than stack 
    std::vector< StackFrame > d_stk;

    std::vector< uint8_t > * d_buf;
    
    void* bufAtOffset( size_t offs ) { return &( (*d_buf)[ offs ] ); }
    void stackIncrementSz( int32_t sz ) { d_stk.back().size+= sz; }
    void stackPop() 
    {
        if( d_stk.empty() ) return;
        int32_t sz = d_stk.back().size;
        *((int32_t*)bufAtOffset(d_stk.back().sizeOffset)) = sz; // updating the size in the buffer
        d_stk.resize( d_stk.size()-1 );

        if( !d_stk.empty() ) d_stk.back().size+= sz; // incrementing size on stack
    }
    void stackPush( bool isArr) 
    {
        int32_t newSz;
        size_t  szOffset;
        if( d_stk.empty() ) {
            newSz = 4;
        } else {
            newSz = 1+4;
            d_buf->push_back( isArr? 0x4 : 0x3 );
        }
        szOffset = d_buf->size();
        d_stk.push_back( { newSz, d_buf->size() } );
    }
    Encoder( const Encoder& ) = delete;

    void* newBytes( size_t addSz ) 
    {
        size_t offs = d_buf->size(); 
        d_buf->resize( d_buf->size() + addSz ) ;
        return bufAtOffset(offs);
    }

    int32_t encodeName( const char* n )
    {
        if( n ) {
            size_t addSz = strlen(n) +1 ;  
            memcpy( newBytes(addSz), n, addSz );
            return addSz;
        } else 
            return 0;
    }
public:
    Encoder() : d_buf(0) {}
    Encoder(std::vector< uint8_t >& buf) : d_buf(&buf) 
        {}

    void init() { 
        d_stk.clear(); 
    }
    void setBuf( std::vector< uint8_t >*  buf ) 
    { 
        d_buf = buf; 
        init();
    }

    void encode_double( double i, const char* name = 0 ) 
    {
        d_buf->push_back( 0x01 );
        int32_t sz = 1 + encodeName(name) + sizeof(double);

        *( (double*)newBytes( sizeof(double) ) ) = i;
        stackIncrementSz(sz);
    }
    void encode_int32( int32_t i, const char* name = 0 ) 
    {
        d_buf->push_back( 0x10 );
        int32_t sz = 1 + encodeName(name) + 4;

        *( (int32_t*)newBytes(4) ) = i;
        stackIncrementSz(sz);
    }
    void encode_bool( bool i, const char* name = 0 ) 
    {
        d_buf->push_back( 0x8 );
        int32_t sz = 1 + encodeName(name) + 4;

        *( (uint8_t*)newBytes(1) ) = ( i ? 0x1: 0x0 );
        stackIncrementSz(sz);
    }
    void encode_string( const char* i, const char* name = 0 ) 
    {
        d_buf->push_back( 0x2 );
        int32_t sz = 1 + encodeName(name) + 4;

        *( (uint8_t*)newBytes(1) ) = ( i ? 0x1: 0x0 );
        stackIncrementSz(sz);
    }
    void document_start( bool isArr=false, const char* name = 0 ) 
    {
        stackPush(isArr);
        if( name ) 
            d_stk.back().size += encodeName(name);
    }
    void document_end( ) { stackPop(); }
};

} // namespace bson
