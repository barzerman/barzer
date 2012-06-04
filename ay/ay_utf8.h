#ifndef AY_UTF8_H
#define AY_UTF8_H

#include <vector>
#include <iterator>
#include <cstddef>
#include <boost/bind.hpp>
#include <stdint.h>
#include <ay/ay_headers.h>

namespace ay
{
	// Represents a single glyph.
	class CharUTF8
	{
		union {
			char        c4[4];
			uint32_t    u4;
		} d_data;
        /// NEVER add anything between d_null and d_data
        const uint8_t d_null; // this should ALWAYS be 0 - thus we achieve 0-termination

		uint8_t d_size;
	public:
        void assign(  const char *ss )
        {
            d_size   =0;
            d_data.u4=0;

            const uint8_t *s = (const uint8_t*) ss;
            s[0]                && (d_data.c4[0]=s[0],d_size=1,s[1]) &&
            (s[1]>>5) >= 0x6    && (d_data.c4[1]=s[1],d_size=2,s[2]) &&
            (s[2]>>4) >= 0xE    && (d_data.c4[2]=s[2],d_size=3,s[3]) &&
            (s[3]>>3) >= 0x1E   && (d_data.c4[3]=s[3],d_size=4);
        }
        void assign(  const char *ss, size_t s_sz )
        {
            d_size   =0;
            d_data.u4=0;

            const uint8_t *s = (const uint8_t *) ss;
            s[0]                && (d_data.c4[0]=s[0],d_size=1,s[1]&&d_size<s_sz) &&
            (s[1]>>5) >= 0x6    && (d_data.c4[1]=s[1],d_size=2,s[2]&&d_size<s_sz) &&
            (s[2]>>4) >= 0xE    && (d_data.c4[2]=s[2],d_size=3,s[3]&&d_size<s_sz) &&
            (s[3]>>3) >= 0x1E   && (d_data.c4[3]=s[3],d_size=4);
        }

		CharUTF8() : d_null(0),d_size(0) { d_data.u4=0; }
		CharUTF8 (const char *s) : d_null(0)
            { assign(s); }
		CharUTF8 (const char *s, size_t s_sz) : d_null(0), d_size(0) 
            { assign(s,s_sz); }

		CharUTF8 (const CharUTF8& o): d_null(0), d_size(o.d_size) 
            { d_data.u4 = o.d_data.u4; }

		CharUTF8& operator= (const char *s) { return(assign(s),*this); }
		CharUTF8& operator= (const CharUTF8& o) 
            { return( d_size=o.d_size, d_data.u4=o.d_data.u4, *this ); }

		bool operator== (const CharUTF8& o) const { return ( d_data.u4 == o.d_data.u4); }
		bool operator<  (const CharUTF8& o) const { return ( d_data.u4 < o.d_data.u4); }

		inline size_t size() const { return d_size; }
		const char* c_str()  const { return d_data.c4; }

		const char* getBuf() const { return d_data.c4; }
		const char* getBuf_end() const { return &(d_data.c4[d_size]); }

        operator const char* () { return d_data.c4; }

        char getChar0() const { return d_data.c4[0]; }
        char getChar1() const { return d_data.c4[1]; }
        char getChar2() const { return d_data.c4[2]; }
        char getChar3() const { return d_data.c4[3]; }
        
        bool isAscii() const { return( d_size== 1 && isascii(d_data.c4[0]) ); }
	};

	// Represents a UTF-8 string.
	class StrUTF8
	{
		std::vector<char> m_buf;

		// Vector of positions: m_positions [i] is the beginning
		// byte of the i'th glyph in the m_buf.
		std::vector<size_t> m_positions;

		inline void appendZero()
		{
			m_buf.push_back(0);
			m_positions.push_back(m_buf.size() - 1);
		}

		inline void removeZero()
		{
            if( m_buf.size() )
			    m_buf.pop_back();
            if( m_positions.size() )
			    m_positions.pop_back();
		}

		inline void addSymbol(const CharUTF8& t)
		{
			m_positions.push_back(m_buf.size());
			m_buf.insert(m_buf.end(), t.getBuf(), t.getBuf_end() );
		}
	public:
        const char* getGlyphStart( size_t g ) const
            { return &m_buf[ m_positions[g] ]; }
        const char* getGlyphEnd( size_t g ) const
            { return ( (g+1)< m_positions.size() ? &(m_buf[ m_positions[g+1]]): &(m_buf[m_buf.size()]) ); }

		class iterator;
		class const_iterator;
		class MutableProxy {
			friend class StrUTF8;
			friend class StrUTF8::iterator;

			StrUTF8 *m_str;
			size_t   m_glyph;

			mutable CharUTF8 m_cachedChar;

			inline MutableProxy(size_t glyphPos, StrUTF8 *str) : 
                m_str(str) , m_glyph(glyphPos) { }

			inline void updatePos(size_t newGlyph)
                { m_glyph = newGlyph; }
		public:
			inline MutableProxy& operator= (const CharUTF8& ch)
                { return( m_str->setGlyph(m_glyph, ch), *this ); }

			inline MutableProxy& operator= (const MutableProxy& other)
                { return( m_str->setGlyph (m_glyph, other), *this ); }

			inline operator CharUTF8 () const
                { return m_str->getGlyph (m_glyph); }

			inline size_t size () const
                { return m_str->getGlyph (m_glyph).size (); }

			inline const char* getBuf () const
                { return m_str->getGlyph(m_glyph).getBuf(); }

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

			inline size_t glyphNum() const
			    { return m_glyph; }

			inline bool isEqual (const MutableProxy& other) const
                { return (m_str == other.m_str && m_glyph == other.m_glyph); }

			inline bool isLess (const MutableProxy& other) const
                { return ( m_str != other.m_str ?  m_str < other.m_str : m_glyph < other.m_glyph); }
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
            inline T* t_ptr() { return static_cast<T*> (this); }
            inline const T* t_const_ptr() const { return static_cast<const T*> (this); }
			inline T& operator++ ()
                { T *t = t_ptr(); return t->setSymb (t->getSymb () + 1); }

			inline T operator++ (int)
                { T t = *t_ptr(); return t.setSymb (t.getSymb () + 1); }

			inline T& operator-- ()
                { T *t = t_ptr(); return t->setSymb (t->getSymb () - 1); }

			inline T operator-- (int)
                { T t = *t_ptr(); return t.setSymb (t.getSymb () - 1); }

			inline T& operator+= (int j)
                { T *t = t_ptr(); return t->setSymb (t->getSymb () + j); }

			inline T& operator-= (int j)
                { T *t = t_ptr(); return t->setSymb (t->getSymb () - j); }

			inline T operator+ (int j) const
                { T t = *t_const_ptr(); return t.setSymb (t.getSymb () + j); }

			inline T operator- (int j) const
                { T t = *t_const_ptr(); return t.setSymb (t.getSymb () - j); }

			inline ptrdiff_t operator- (const T& other) const
			{
				const T *thisT = static_cast<const T*> (this);
				const T *otherT = static_cast<const T*> (&other);
				return thisT->getSymb () - otherT->getSymb ();
			}
		};

		class iterator : public CmpMixin<iterator> , public DiffMixin<iterator> {
			MutableProxy m_proxy;

			friend class const_iterator;
			friend struct DiffMixin<iterator>;
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
                { return m_proxy; }

			inline MutableProxy* operator-> ()
                { return &m_proxy; }

			inline bool operator== (const iterator& other) const
                { return m_proxy.isEqual (other.m_proxy); }

			inline bool operator< (const iterator& other) const
                { return m_proxy.isLess (other.m_proxy); }
		private:
			inline size_t getSymb () const
                { return m_proxy.m_glyph; }

			inline iterator& setSymb (size_t glyph)
                { return( m_proxy.m_glyph = glyph,*this); }
		};

		class const_iterator : public CmpMixin<const_iterator> , public DiffMixin<const_iterator> {
			const StrUTF8 *m_str;
			size_t m_pos;

			mutable CharUTF8 m_reqChar;

			friend struct DiffMixin<const_iterator>;
		public:
			typedef std::random_access_iterator_tag iterator_category;
			typedef CharUTF8 value_type;
			typedef const CharUTF8& reference;
			typedef const CharUTF8 *pointer;
			typedef ptrdiff_t difference_type;

			inline const_iterator (const StrUTF8 *str = 0, size_t pos = 0) : m_str (str) , m_pos (pos) { }

			inline explicit const_iterator (const iterator& iterator) : 
                m_str (iterator.m_proxy.m_str) , m_pos (iterator.m_proxy.m_glyph) { }

			inline CharUTF8 operator* () const
                { return m_str->getGlyph (m_pos); }

			inline CharUTF8* operator-> () const
                { return ( m_reqChar = **this, &m_reqChar ); }

			inline bool operator== (const const_iterator& other) const
                { return (m_str == other.m_str && m_pos == other.m_pos); }

			inline bool operator< (const const_iterator& other) const
                { return m_pos < other.m_pos; }
		private:
			inline size_t getSymb () const { return m_pos; }

			inline const_iterator& setSymb(size_t glyph) { return( m_pos = glyph,*this); }
		};

		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        StrUTF8& assign( const char*s, const char* s_end=0 )
        {
		    const char *begin = s;
		    size_t lastPos = 0;
            if( !s_end ) 
                s_end = s+strlen(s);

		    while(s< s_end) {
			    const size_t glyphSize = CharUTF8(s).size();
			    m_positions.push_back (lastPos);
			    lastPos += glyphSize;
			    s += glyphSize;
		    }
    
		    m_buf.reserve (lastPos);
		    std::copy (begin, s, std::back_inserter (m_buf));
		    appendZero ();
            return *this;
        }
        StrUTF8& assign( const StrUTF8& o, size_t fromGlyph, size_t numGlyphs ) 
            { return assign( o.getGlyphStart(fromGlyph), o.getGlyphEnd(fromGlyph+numGlyphs) ); }

		StrUTF8() {appendZero ();}

		StrUTF8(const char*s)
            { assign(s); }

		StrUTF8(const char*s, size_t s_sz )
            { assign(s,s+s_sz); }

		template<typename Iter>
		StrUTF8 (Iter begin, Iter end)
		{
			std::for_each(begin, end, boost::bind (&ay::StrUTF8::addSymbol, this, _1));
			appendZero ();
		}

		const char*     c_str() const           { return &m_buf [0]; }
		inline size_t   size() const            { return m_positions.size () - 1; }
		inline size_t   getGlyphCount() const   { return size(); }
		inline size_t   bytesCount() const      { return m_buf.size (); }

		inline void clear() {
            m_buf.clear();
            m_positions.clear();
            appendZero();
        }

		inline MutableProxy operator[] (size_t pos) { return MutableProxy(pos, this); }

		inline CharUTF8 operator[] (size_t glyphNum) const { return getGlyph(glyphNum); }

		inline CharUTF8 getGlyph(size_t glyphNum) const
		{
			const size_t pos = m_positions [glyphNum];
			return CharUTF8(&m_buf[pos],m_positions[glyphNum+1]-m_positions[glyphNum]);
		}

		inline void setGlyph (size_t glyphNum, const CharUTF8& glyph)
		{
			const size_t pos = m_positions [glyphNum];
			const size_t wasEnd = m_positions [glyphNum + 1];
			const size_t wasSize = wasEnd - pos;
			m_buf.erase (m_buf.begin () + pos, m_buf.begin () + wasEnd);

			const ptrdiff_t diff = wasSize - glyph.size ();
			m_buf.insert(m_buf.begin () + pos, glyph.getBuf(), glyph.getBuf_end() );

			if (diff) {
				for (std::vector<size_t>::iterator i = m_positions.begin () + glyphNum + 1, end = m_positions.end (); i != end; ++i)
					*i += diff;
            }
		}

		inline void push_back (const value_type& t)
		{
			removeZero();
			addSymbol(t);
			appendZero();
		}

		inline iterator begin ()                { return iterator (this, 0); }
		inline iterator end ()                  { return iterator (this, size ()); }

		inline iterator ith (size_t i)                { return iterator (this, i); }

		inline const_iterator begin () const    { return const_iterator (this, 0); }
		inline const_iterator end () const      { return const_iterator (this, size ()); }

		inline reverse_iterator rbegin ()       { return reverse_iterator (end ()); }
		inline reverse_iterator rend ()         { return reverse_iterator (begin ()); }

		inline const_reverse_iterator rbegin () const   { return const_reverse_iterator (end ()); }
		inline const_reverse_iterator rend () const     { return const_reverse_iterator (begin ()); }

		void swap (iterator x, iterator y)
        {
            const CharUTF8& tmp = *x;
            *x = *y;
            *y = tmp;
        }
		void swap (size_t x, size_t y)
            { swap( ith(x), ith(y) ); }
	};

	//StrUTF8 operator+ (const StrUTF8&, const StrUTF8&);
}

#endif
