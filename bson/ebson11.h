/* 2013 Barzer 

EBSON11 v1.0 - Encoder of BSON in c++11 

http://bsonspec.org/#/specification
implements most common types (not all)
*/
/**********************************************************************
 * eBSON11 — BSON encoder in C++11.
 *
 * Copyright (C) 2013  Georg Rudoy		<georg@barzer.net>
 * Copyright (C) 2013  Andre Yanpolsky	<andre@barzer.net>
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <iostream>

namespace ebson11 {
namespace detail {
    /// this is a service type to be used in place of std::vector for performance reasons 
    /// it doesnt initialize the vector (especially on resize()) - about 50% performance gain
	template<typename T, typename Alloc = std::allocator<T>>
	class uninit_vector
	{
		T *m_data = nullptr;
		size_t m_capacity = 0;
		size_t m_size = 0;
	public:
		typedef uint8_t value_type;

		uninit_vector() { }

		~uninit_vector() { delete [] m_data; }

		uninit_vector(const uninit_vector<T>& other)
		{
			reserve(other.m_size);
			m_size = other.m_size;
			memcpy(m_data, other.m_data, other.m_size * sizeof(T));
		}

		uninit_vector(uninit_vector<T>&& other) { swap(other); }

		uninit_vector& operator=(const uninit_vector<T>& other)
		{
			reserve(other.m_size);
			memcpy(m_data, other.m_data, other.m_size * sizeof(T));
		}

		uninit_vector& operator=(uninit_vector<T>&& other) { swap(other); }

		const T* begin()    const { return m_data; }

		const T* end()      const { return m_data + m_size; }

		void swap(uninit_vector& other)
		{
			std::swap(other.m_data, m_data);
			std::swap(other.m_capacity, m_capacity);
			std::swap(other.m_size, m_size);
		}

		void reserve(size_t capacity)
		{
			if (capacity <= m_capacity)
				return;

			uninit_vector<T> other;
			other.m_data = new T[capacity];
			other.m_capacity = capacity;
			other.m_size = m_size;

			if (m_data)
				memcpy(other.m_data, m_data, sizeof(T) * m_size);

			swap(other);
		}

		void resize(size_t size)
		{
			reserve(size);
			m_size = size;
		}

		void push_back(const T& t)
		{
			if (m_size == m_capacity)
				reserve(m_capacity * 2);

			m_data[m_size++] = t;
		}

		const T& operator[](size_t p) const { return m_data[p]; }

		T& operator[](size_t p) { return m_data[p]; }

		size_t size() const { return m_size; }

		std::vector<T> to_std_vector() const
		{
			std::vector<T> result;
			result.resize(size());
			memcpy(&result[0], m_data, sizeof(T) * size());
			return result;
		}

		bool operator==(const uninit_vector<T>& other) const
		{
			return m_size == other.m_size &&
					!memcmp(m_data, other.m_data, m_size * sizeof(T));
		}

		bool operator==(const std::vector<T>& other) const
		{
			return m_size == other.size() &&
					!memcmp(m_data, &other[0], m_size * sizeof(T));
		}
	};
} // namespace detail

template<template<typename, typename> class BufType = detail::uninit_vector>
class EncoderT {
public:
	typedef BufType<uint8_t, std::allocator<uint8_t>> BufType_t;
private:
    struct StackFrame
    {
        int32_t   size          = 0; // cumulative size
        size_t    sizeOffset    = 0;      // &buf[sizeOffset] - size of the document 

        StackFrame() {}
        StackFrame( int32_t sz, size_t szOffs) : size(sz), sizeOffset(szOffs) {}
    };
    std::vector<StackFrame> d_stk;

    BufType_t d_buf;
    
    void* bufAtOffset( size_t offs ) { return &(d_buf[offs]); }
    void stackIncrementSz( int32_t sz ) { d_stk.back().size+= sz; }

    void* newBytes( size_t addSz ) 
    {
        size_t offs = d_buf.size();
        d_buf.resize( d_buf.size() + addSz ) ;
        return bufAtOffset(offs);
    }
    void stackPop() 
    {
		if (d_stk.empty())
			return;

		*static_cast<uint8_t*>(newBytes(1)) = 0;
		stackIncrementSz(1);

		const int32_t sz = d_stk.back().size;

		*((int32_t*)bufAtOffset(d_stk.back().sizeOffset)) = sz; // updating the size in the buffer
		d_stk.resize(d_stk.size() - 1);

		if (!d_stk.empty())
			d_stk.back().size += sz; // incrementing size on stack
    }
    void    stackPush()
    {
        int32_t newSz = 4; // length is always there
        d_stk.push_back({ newSz, d_buf.size() });
		newBytes(sizeof(uint32_t));
    }

    int32_t encodeName( const char* n )
    {
		if (!n) n = "";

		const size_t addSz = std::strlen(n) +1 ;
		std::memcpy( newBytes(addSz), n, addSz );
		return addSz;
    }

    template<typename T>
    void encode_type(T t, uint8_t typeId, const char *name)
	{
		d_buf.push_back(typeId);
		const int32_t sz = 1 + encodeName(name) + sizeof(T);
		*(static_cast<T*>(newBytes(sizeof(T)))) = t;
		stackIncrementSz(sz);
	}
    EncoderT(const EncoderT&) = delete;
public:
    enum { DEFAULT_RESERVE_SZ = 1024*64 };

    EncoderT(size_t reserve = DEFAULT_RESERVE_SZ)
        { d_buf.reserve(reserve); stackPush(); }

    void start() 
    {
        d_buf.resize(0);
        d_stk.clear();
        stackPush();
    }

    const BufType_t& finalize() { return ( stackPop(), d_buf ); }


    // individual value encoders
    void encode_double( double i, const char* name = 0 ) 
    {
		static_assert(sizeof(double) == 8, "we don't support non-8-bytes-doubles yet");
		encode_type(i, 0x01, name);
    }

    void encode_int32( int32_t i, const char* name = 0 ) { encode_type(i, 0x10, name); }
    void encode_bool( bool i, const char* name = 0 )     { encode_type(static_cast<uint8_t>(i), 0x08, name); }

    void encode_string( const char *str, const char* name = 0 )
    {
		d_buf.push_back(0x2);

		const auto strlen = static_cast<int32_t>(std::strlen(str));
        const int32_t sumSz = 1 + encodeName(name) + 4 + strlen + 1;

		auto mem = newBytes(4 + strlen + 1);
		*static_cast<uint32_t*>(mem) = (strlen + 1);
		mem = static_cast<char*>(mem) + 4;
		memcpy(mem, str, strlen);
		mem = static_cast<char*>(mem) + strlen;
		*static_cast<uint8_t*>(mem) = 0;
		stackIncrementSz(sumSz);
    }
    // document or array begin (pushes the stack)
    void document_start(bool isArr=false, const char* name = 0)
    {
		d_buf.push_back(isArr ? 0x4 : 0x3);
		d_stk.back().size += encodeName(name) + 1;
        stackPush();
    }
    // document or array end (pops the stack)
    void document_end( ) { stackPop(); }
};

typedef EncoderT<> Encoder;
struct EncoderRaii {
    Encoder& encoder;
    EncoderRaii( const EncoderRaii& ) = delete;
    EncoderRaii( Encoder& e, bool isArr=false, const char* name = 0 ) : encoder(e) 
        { encoder.document_start(isArr, name ); }
    ~EncoderRaii()
        { encoder.document_end(); }
};

} // namespace ebson11
