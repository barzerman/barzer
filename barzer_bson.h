#pragma once 
#include <string>
#include <vector>
#include <cstring>
#include <iostream>

namespace bson
{
namespace detail
{
	template<typename T, typename Alloc = std::allocator<T>>
	class uninit_vector
	{
		T *m_data = nullptr;
		size_t m_capacity = 0;
		size_t m_size = 0;
	public:
		typedef uint8_t value_type;

		uninit_vector()
		{
		}

		~uninit_vector()
		{
			delete [] m_data;
		}

		uninit_vector(const uninit_vector<T>& other)
		{
			reserve(other.m_size);
			memcpy(m_data, other.m_data, other.m_size * sizeof(T));
		}

		uninit_vector(uninit_vector<T>&& other)
		{
			swap(other);
		}

		uninit_vector& operator=(const uninit_vector<T>& other)
		{
			reserve(other.m_size);
			memcpy(m_data, other.m_data, other.m_size * sizeof(T));
		}

		uninit_vector& operator=(uninit_vector<T>&& other)
		{
			swap(other);
		}

		const T* begin() const
		{
			return m_data;
		}

		const T* end() const
		{
			return m_data + m_size;
		}

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

		const T& operator[](size_t p) const
		{
			return m_data[p];
		}

		T& operator[](size_t p)
		{
			return m_data[p];
		}

		size_t size() const
		{
			return m_size;
		}

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
					m_capacity == other.m_capacity &&
					!memcmp(m_data, other.m_data, m_size * sizeof(T));
		}

		bool operator==(const std::vector<T>& other) const
		{
			return m_size == other.size() &&
					!memcmp(m_data, &other[0], m_size * sizeof(T));
		}
	};
}

template<template<typename, typename> class BufType = detail::uninit_vector>
class EncoderT
{
public:
	typedef BufType<uint8_t, std::allocator<uint8_t>> BufType_t;
private:
    struct StackFrame
    {
        int32_t   size; // cumulative size
        size_t    sizeOffset;      // &buf[sizeOffset] - size of the document 

        StackFrame() : size(0), sizeOffset(0) {}
        StackFrame( int32_t sz, size_t szOffs) : size(sz), sizeOffset(szOffs) {}
    };
    std::vector<StackFrame> d_stk;

    BufType_t d_buf;
    
    void* bufAtOffset( size_t offs ) { return &(d_buf[offs]); }
    void stackIncrementSz( int32_t sz ) { d_stk.back().size+= sz; }
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

    void stackPush( bool isArr) 
    {
        int32_t newSz = 4;
        if (!d_stk.empty())
		{
			++newSz;
            d_buf.push_back(isArr ? 0x4 : 0x3);
        }
        d_stk.push_back({ newSz, d_buf.size() });
		newBytes(sizeof(uint32_t));
    }

    void* newBytes( size_t addSz ) 
    {
        size_t offs = d_buf.size();
        d_buf.resize( d_buf.size() + addSz ) ;
        return bufAtOffset(offs);
    }

    int32_t encodeName( const char* n )
    {
		if (!n)
			n = "";

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
public:
    EncoderT(size_t reserve = 100000)
	{
		d_buf.reserve(reserve);
        stackPush(false);
	}

    EncoderT(const EncoderT&) = delete;

	const BufType_t& finalize()
	{
		stackPop();
		return d_buf;
	}

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
    void document_start(bool isArr=false, const char* name = 0)
    {
        stackPush(isArr);
		d_stk.back().size += encodeName(name);
    }
    void document_end( ) { stackPop(); }
};

typedef EncoderT<> Encoder;
} // namespace bson
