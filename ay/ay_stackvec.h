#pragma once

#include <vector>
#include <iostream>

namespace ay
{
	template<typename T, template<typename T, typename Alloc> class Cont = std::vector, typename Alloc = std::allocator<T>>
	class StackVec
	{
		typedef Cont<T, Alloc> ContClass;
		enum
		{
			ObjSize = sizeof(ContClass),
			SSize = ObjSize / sizeof(T)
		};

		size_t m_size;
		union
		{
			char m_rawData[ObjSize];
			T m_objs[SSize];
		} m_u;
	protected:
		inline const ContClass* cont() const { return reinterpret_cast<const ContClass*>(m_u.m_rawData); }
		inline ContClass* cont() { return reinterpret_cast<ContClass*>(m_u.m_rawData); }
		
		inline void dealloc() { cont()->~ContClass(); }

		inline bool isEx() const { return m_size > SSize; }
	public:
		StackVec()
		: m_size(0)
		{
		}

		~StackVec()
		{
			if (isEx())
				dealloc();
		}

		StackVec(const StackVec& sv)
		{
			*this = sv;
		}

		StackVec& operator=(const StackVec& sv)
		{
			m_size = sv.m_size;
			if (isEx())
				new (m_u.m_rawData) ContClass(*sv.cont());
			else
				std::copy(sv.m_u.m_objs, sv.m_u.m_objs + m_size, m_u.m_objs);

			return *this;
		}

		void push_back(const T& t)
		{
			if (m_size < SSize)
				m_u.m_objs[m_size] = t;
			else if (m_size > SSize)
				cont()->push_back(t);
			else
			{
				ContClass c;
				std::copy(m_u.m_objs, m_u.m_objs + SSize, std::back_inserter(c));
				new (m_u.m_rawData) ContClass(c);
				cont()->push_back(t);
			}

			++m_size;
		}

		void pop_back()
		{
			if (m_size > SSize + 1)
				cont()->pop_back();
			else if (m_size == SSize + 1)
			{
				cont()->pop_back();
				const ContClass c = *cont();
				dealloc();
				std::copy(c.begin(), c.end(), m_u.m_objs);
			}

			--m_size;
		}

		inline size_t size() const { return m_size; }

		inline T& operator[](size_t pos)
		{
			return isEx() ? (*cont())[pos] : m_u.m_objs[pos];
		}

		inline const T& operator[](size_t pos) const
		{
			return isEx() ? (*cont())[pos] : m_u.m_objs[pos];
		}
	};
}
