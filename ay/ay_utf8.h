#ifndef AY_UTF8_H
#define AY_UTF8_H

#include <vector>
#include <iterator>
#include <cstddef>
#include "ay_headers.h"

namespace ay
{
	// Represents a single glyph.
	class CharUTF8
	{
		size_t m_size;

		enum { MaxBytes = 4 };
		union
		{
			char m_buf [MaxBytes];
			u_int32_t m_int;
		};
	public:
		CharUTF8 ();
		CharUTF8 (const char *beginning);
		CharUTF8 (const char *beginning, size_t size);
		CharUTF8 (const CharUTF8& other);

		CharUTF8& operator= (const char *beginning);
		CharUTF8& operator= (const CharUTF8& other);

		bool operator== (const CharUTF8& other) const;
		bool operator< (const CharUTF8& other) const;

		inline size_t size () const
		{
			return m_size;
		}
		const char* getBuf () const;
	};

	// Represents a UTF-8 string.
	class StrUTF8
	{
		std::vector<char> m_buf;

		// Vector of positions: m_positions [i] is the beginning
		// byte of the i'th glyph in the m_buf.
		std::vector<size_t> m_positions;

		inline void appendZero ()
		{
			m_buf.push_back (0);
			m_positions.push_back (m_buf.size () - 1);
		}

		inline void removeZero ()
		{
			m_buf.pop_back ();
			m_positions.pop_back ();
		}
	public:
		class iterator;
		class const_iterator;
		class MutableProxy
		{
			friend class StrUTF8;
			friend class StrUTF8::iterator;

			StrUTF8 *m_str;
			size_t m_glyph;

			mutable CharUTF8 m_cachedChar;

			inline MutableProxy (size_t glyphPos, StrUTF8 *str)
			: m_str (str)
			, m_glyph (glyphPos)
			{
			}

			inline void updatePos (size_t newGlyph)
			{
				m_glyph = newGlyph;
			}
		public:
			inline MutableProxy& operator= (const CharUTF8& ch)
			{
				m_str->setGlyph (m_glyph, ch);
				return *this;
			}

			inline MutableProxy& operator= (const MutableProxy& other)
			{
				m_str->setGlyph (m_glyph, other);
				return *this;
			}

			inline operator CharUTF8 () const
			{
				return m_str->getGlyph (m_glyph);
			}

			inline size_t size () const
			{
				return m_str->getGlyph (m_glyph).size ();
			}

			inline const char* getBuf () const
			{
				return m_str->getGlyph (m_glyph).getBuf ();
			}

			inline CharUTF8& getCachedChar ()
			{
				m_cachedChar = *this;
				return m_cachedChar;
			}

			inline const CharUTF8& getCachedChar () const
			{
				m_cachedChar = *this;
				return m_cachedChar;
			}

			inline size_t glyphNum () const
			{
				return m_glyph;
			}

			inline bool isEqual (const MutableProxy& other) const
			{
				return m_str == other.m_str &&
						m_glyph == other.m_glyph;
			}

			inline bool isLess (const MutableProxy& other) const
			{
				return m_str != other.m_str ?
						m_str < other.m_str :
						m_glyph < other.m_glyph;
			}
		};

		typedef CharUTF8 value_type;
		typedef CharUTF8& reference;
		typedef const CharUTF8& const_reference;

		template<typename T>
		struct CmpMixin
		{
			inline bool operator!= (const T& other) const
			{
				const T *t = static_cast<const T*> (this);
				return !t->operator== (other);
			}

			inline bool operator<= (const T& other) const
			{
				const T *t = static_cast<const T*> (this);
				return t->operator< (other) || t->operator== (other);
			}

			inline bool operator> (const T& other) const
			{
				const T *t = static_cast<const T*> (this);
				return !t->operator<= (other);
			}

			inline bool operator>= (const T& other) const
			{
				const T *t = static_cast<const T*> (this);
				return !t->operator< (other);
			}
		};

		template<typename T>
		struct DiffMixin
		{
			inline T& operator++ ()
			{
				T *t = static_cast<T*> (this);
				t->setSymb (t->getSymb () + 1);
				return *t;
			}

			inline T operator++ (int)
			{
				T t = *static_cast<T*> (this);
				t.setSymb (t.getSymb () + 1);
				return t;
			}

			inline T& operator-- ()
			{
				T *t = static_cast<T*> (this);
				t->setSymb (t->getSymb () - 1);
				return *t;
			}

			inline T operator-- (int)
			{
				T t = *static_cast<T*> (this);
				t.setSymb (t.getSymb () - 1);
				return t;
			}

			inline T& operator+= (int j)
			{
				T *t = static_cast<T*> (this);
				t->setSymb (t->getSymb () + j);
				return *t;
			}

			inline T& operator-= (int j)
			{
				T *t = static_cast<T*> (this);
				t->setSymb (t->getSymb () - j);
				return *t;
			}

			inline T operator+ (int j) const
			{
				T t = *static_cast<const T*> (this);
				t.setSymb (t.getSymb () + j);
				return t;
			}

			inline T operator- (int j) const
			{
				T t = *static_cast<const T*> (this);
				t.setSymb (t.getSymb () - j);
				return t;
			}

			inline ptrdiff_t operator- (const T& other) const
			{
				const T *thisT = static_cast<const T*> (this);
				const T *otherT = static_cast<const T*> (&other);
				return thisT->getSymb () - otherT->getSymb ();
			}
		};

		class iterator : public CmpMixin<iterator>
					   , public DiffMixin<iterator>
		{
			MutableProxy m_proxy;

			friend class const_iterator;
			friend class DiffMixin<iterator>;
		public:
			typedef std::random_access_iterator_tag iterator_category;
			typedef MutableProxy value_type;
			typedef MutableProxy& reference;
			typedef MutableProxy *pointer;
			typedef ptrdiff_t difference_type;

			inline iterator (StrUTF8 *str = 0, size_t pos = 0)
			: m_proxy (pos, str)
			{
			}

			inline MutableProxy& operator* ()
			{
				return m_proxy;
			}

			inline MutableProxy* operator-> ()
			{
				return &m_proxy;
			}

			inline bool operator== (const iterator& other) const
			{
				return m_proxy.isEqual (other.m_proxy);
			}

			inline bool operator< (const iterator& other) const
			{
				return m_proxy.isLess (other.m_proxy);
			}
		private:
			inline size_t getSymb () const
			{
				return m_proxy.m_glyph;
			}

			inline void setSymb (size_t glyph)
			{
				m_proxy.m_glyph = glyph;
			}
		};

		class const_iterator : public CmpMixin<const_iterator>
							 , public DiffMixin<const_iterator>
		{
			const StrUTF8 *m_str;
			size_t m_pos;

			mutable CharUTF8 m_reqChar;

			friend class DiffMixin<const_iterator>;
		public:
			typedef std::random_access_iterator_tag iterator_category;
			typedef CharUTF8 value_type;
			typedef const CharUTF8& reference;
			typedef const CharUTF8 *pointer;
			typedef ptrdiff_t difference_type;

			inline const_iterator (const StrUTF8 *str = 0, size_t pos = 0)
			: m_str (str)
			, m_pos (pos)
			{
			}

			inline explicit const_iterator (const iterator& iterator)
			: m_str (iterator.m_proxy.m_str)
			, m_pos (iterator.m_proxy.m_glyph)
			{
			}

			inline CharUTF8 operator* () const
			{
				return m_str->getGlyph (m_pos);
			}

			inline CharUTF8* operator-> () const
			{
				m_reqChar = **this;
				return &m_reqChar;
			}

			inline bool operator== (const const_iterator& other) const
			{
				return m_str == other.m_str &&
						m_pos == other.m_pos;
			}

			inline bool operator< (const const_iterator& other) const
			{
				return m_pos < other.m_pos;
			}
		private:
			inline size_t getSymb () const
			{
				return m_pos;
			}

			inline void setSymb (size_t glyph)
			{
				m_pos = glyph;
			}
		};

		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		StrUTF8 ();
		StrUTF8 (const char*);
		StrUTF8 (const StrUTF8&);

		const char* c_str () const { return &m_buf [0]; }
		inline size_t size () const { return m_positions.size () - 1; }
		inline size_t bytesCount () const { return m_buf.size (); }

		void clear ();

		inline MutableProxy operator[] (size_t pos)
		{
			return MutableProxy (pos, this);
		}

		inline CharUTF8 operator[] (size_t glyphNum) const
		{
			return getGlyph (glyphNum);
		}

		inline CharUTF8 getGlyph (size_t glyphNum) const
		{
			const size_t pos = m_positions [glyphNum];
			return CharUTF8 (&m_buf [pos], m_positions [glyphNum + 1] - pos);
		}

		inline void setGlyph (size_t glyphNum, const CharUTF8& glyph)
		{
			const size_t pos = m_positions [glyphNum];
			const size_t wasEnd = m_positions [glyphNum + 1];
			const size_t wasSize = wasEnd - pos;
			m_buf.erase (m_buf.begin () + pos, m_buf.begin () + wasEnd);

			const char *glyphBuf = glyph.getBuf ();
			const ptrdiff_t diff = wasSize - glyph.size ();
			m_buf.insert (m_buf.begin () + pos, glyphBuf, glyphBuf + glyph.size ());

			if (diff)
				for (std::vector<size_t>::iterator i = m_positions.begin () + glyphNum + 1,
							end = m_positions.end (); i != end; ++i)
					*i += diff;
		}

		/*
		inline StrUTF8& operator+= (const value_type& t) { m_chars.push_back (t); return *this; }
		StrUTF8& operator+= (const StrUTF8&);
		*/

		inline void push_back (const value_type& t)
		{
			removeZero ();

			m_positions.push_back (m_buf.size ());
			const char *buf = t.getBuf ();
			m_buf.insert (m_buf.end (), buf, buf + t.size ());

			appendZero ();
		}

		inline iterator begin () { return iterator (this, 0); }
		inline iterator end () { return iterator (this, size ()); }
		inline const_iterator begin () const { return const_iterator (this, 0); }
		inline const_iterator end () const { return const_iterator (this, size ()); }

		inline reverse_iterator rbegin () { return reverse_iterator (end ()); }
		inline reverse_iterator rend () { return reverse_iterator (begin ()); }
		inline const_reverse_iterator rbegin () const { return const_reverse_iterator (end ()); }
		inline const_reverse_iterator rend () const { return const_reverse_iterator (begin ()); }

		/*
		iterator erase (iterator pos);
		iterator erase (iterator first, iterator last);

		void insert (iterator at, const value_type& c);
		void insert (iterator at, const StrUTF8& str);
		*/

		void swap (iterator, iterator);
	};

	//StrUTF8 operator+ (const StrUTF8&, const StrUTF8&);
}

#endif
