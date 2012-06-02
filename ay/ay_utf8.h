#ifndef AY_UTF8_H
#define AY_UTF8_H

#include <vector>
#include <iterator>

namespace ay
{
	// Represents a single glyph.
	class CharUTF8
	{
		size_t m_size;

		enum { MaxBytes = 4 };
		char m_buf [MaxBytes];
	public:
		CharUTF8 ();
		CharUTF8 (const char *beginning);
		CharUTF8 (const CharUTF8& other);

		CharUTF8& operator= (const char *beginning);
		CharUTF8& operator= (const CharUTF8& other);

		bool operator== (const CharUTF8& other) const;

		inline size_t size () const
		{
			return m_size;
		}
		const char* getBuf () const;
	};

	// Represents a UTF-8 string.
	class StrUTF8
	{
		typedef std::vector<CharUTF8> storage_t;
		storage_t m_chars;
	public:
		typedef CharUTF8 value_type;
		typedef CharUTF8& reference;
		typedef const CharUTF8& const_reference;

		typedef storage_t::iterator iterator;
		typedef storage_t::const_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		StrUTF8 ();
		StrUTF8 (const char *string);

		inline void clear () { m_chars.clear (); }

		inline CharUTF8& operator[] (size_t pos) { return m_chars [pos]; }
		inline const CharUTF8& operator[] (size_t pos) const { return m_chars [pos]; }
		inline StrUTF8& operator+= (const value_type& t) { m_chars.push_back (t); return *this; }
		StrUTF8& operator+= (const StrUTF8&);

		inline void push_back (const value_type& t) { m_chars.push_back (t); }

		inline iterator begin () { return m_chars.begin (); }
		inline iterator end () { return m_chars.end (); }
		inline const_iterator begin () const { return m_chars.begin (); }
		inline const_iterator end () const { return m_chars.end (); }

		inline reverse_iterator rbegin () { return m_chars.rbegin (); }
		inline reverse_iterator rend () { return m_chars.rend (); }
		inline const_reverse_iterator rbegin () const { return m_chars.rbegin (); }
		inline const_reverse_iterator rend () const { return m_chars.rend (); }

		iterator erase (iterator pos);
		iterator erase (iterator first, iterator last);

		void insert (iterator at, const value_type& c);
		void insert (iterator at, const StrUTF8& str);

		void swapChars (iterator, iterator);

		inline size_t size () const { return m_chars.size (); }
		size_t bytesCount () const;

		char* buildRawStr () const;
		std::vector<char> buildStr () const;
		
		template<typename OutIterator>
		inline static OutIterator buildString (StrUTF8::const_iterator begin, StrUTF8::const_iterator end, OutIterator out)
		{
			for ( ; begin != end; ++begin)
			{
				const char *buf = begin->getBuf ();
				while (*buf)
					++(out = *buf++);
			}
			return out;
		}

		template<typename OutIterator>
		OutIterator buildString (OutIterator out) const
		{
			return buildString (begin (), end (), out);
		}

		inline std::ostream& print (std::ostream& out) const
		{
			buildString (std::ostream_iterator<char> (out));
			return out;
		}
	};

	StrUTF8 operator+ (const StrUTF8&, const StrUTF8&);
}

#endif
