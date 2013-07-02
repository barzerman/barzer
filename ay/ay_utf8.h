
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <vector>
#include <iterator>
#include <cstddef>
#include <string>
#include <stdint.h>
#include "ay_headers.h"

namespace ay
{
	class StrUTF8;

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
            (s[0]>>5) >= 0x6    && (d_data.c4[1]=s[1],d_size=2,s[2]) &&
            (s[0]>>4) >= 0xE    && (d_data.c4[2]=s[2],d_size=3,s[3]) &&
            (s[0]>>3) >= 0x1E   && (d_data.c4[3]=s[3],d_size=4);
        }
        void assign(  const char *ss, size_t s_sz )
        {
            d_size   =0;
            d_data.u4=0;

            const uint8_t *s = (const uint8_t *) ss;
            s[0]                && (d_data.c4[0]=s[0],d_size=1,s[1]&&d_size<s_sz) &&
            (s[0]>>5) >= 0x6    && (d_data.c4[1]=s[1],d_size=2,s[2]&&d_size<s_sz) &&
            (s[0]>>4) >= 0xE    && (d_data.c4[2]=s[2],d_size=3,s[3]&&d_size<s_sz) &&
            (s[0]>>3) >= 0x1E   && (d_data.c4[3]=s[3],d_size=4);
        }

		CharUTF8() : d_null(0),d_size(0) { d_data.u4=0; }
		CharUTF8 (const char *s) : d_null(0)
            { assign(s); }
		CharUTF8 (const char *s, size_t s_sz) : d_null(0), d_size(0) 
            { assign(s,s_sz); }
		CharUTF8 (const char *s, const char* s_end) : d_null(0), d_size(0) 
            { assign(s,(s_end-s)); }

		CharUTF8 (const char o): d_null(0), d_size(o? 1:0) 
            { d_data.c4[0]=o; d_data.c4[1]=d_data.c4[2]=d_data.c4[3]=0; }
		CharUTF8 (const CharUTF8& o): d_null(0), d_size(o.d_size) 
            { d_data.u4 = o.d_data.u4; }

		CharUTF8& operator= (const char *s) { return(assign(s),*this); }
		CharUTF8& operator= (const CharUTF8& o) 
            { return( d_size=o.d_size, d_data.u4=o.d_data.u4, *this ); }

		bool operator== (const CharUTF8& o) const { return ( d_data.u4 == o.d_data.u4); }
		bool operator== (char o) const { return (!d_data.c4[1] && d_data.c4[0] == o); }
		bool operator== (const char* o) const { 
            return (
            ( o[0] == d_data.c4[0]) && (
                !o[0] || (
                    (o[1] == d_data.c4[1]) && ( 
                        !o[1] || (
                            (o[2] == d_data.c4[2]) &&
                            ( !o[2] || ((o[3] == d_data.c4[3]) && (!o[3]||!o[4])) )
                          )
                        )
                    )
                )
            );
        }
        
        bool operator!= (const CharUTF8& o) const { return !(*this == o); }
        bool operator!= (char o) const { return !(*this == o); }
        bool operator!= (const char *o) const { return !(*this == o); }
        
        bool isInStr( const char* s ) const { return ( strstr(s,c_str())!=0 ); }
        bool isPunct() const { return( ispunct(d_data.c4[0]) || isApostrophe() ); }

        bool isPunctOtherThan( const char* s ) const 
            { return( !isInStr(s) || ispunct(d_data.c4[0]) || isApostrophe() ); }
        /// can be a word terminator
        bool isWordTerminator() const 
            { return( (d_data.c4[0]=='.' && !d_data.c4[1]) || isApostrophe() ); }

        bool isApostrophe() const { return isInStr( "`'‘’‛" ); }
		bool operator<  (const CharUTF8& o) const { return ( d_data.u4 < o.d_data.u4); }

		inline size_t size() const { return d_size; }
		const char* c_str()  const { return d_data.c4; }

		const char* getBuf() const { return d_data.c4; }
		const char* getBuf_end() const { return &(d_data.c4[d_size]); }

        operator const char* () const { return d_data.c4; }

        char getChar0() const { return d_data.c4[0]; }
        char getChar1() const { return d_data.c4[1]; }
        char getChar2() const { return d_data.c4[2]; }
        char getChar3() const { return d_data.c4[3]; }

        bool isAscii() const { return( d_size== 1 && isascii(d_data.c4[0]) ); }
        // copies up to the first 0 byte 
        inline void copyToBufNoNull( char* buf ) const 
        {
            
            d_data.c4[0] && ( buf[0]= d_data.c4[0]) &&
            d_data.c4[1] && ( buf[1]= d_data.c4[1]) &&
            d_data.c4[2] && ( buf[2]= d_data.c4[2]) &&
            d_data.c4[3] && ( buf[3]= d_data.c4[3]);
        }
        inline void copyToBufWithNull( char* buf ) const 
            { copyToBufNoNull(buf); buf[d_size]=0; }

		inline uint32_t toUTF32() const
		{
			uint32_t result = 0;
			switch (d_size)
			{
			case 1:
				result = d_data.c4[0];
				break;
			case 2:
				result = ((d_data.c4[0] & 0x1F) << 6) + (d_data.c4[1] & 0x3F);
				break;
			case 3:
				result = ((d_data.c4[0] & 0xF) << 12) + ((d_data.c4[1] & 0x3F) << 6) + (d_data.c4[2] & 0x3F);
				break;
			case 4:
				result = ((d_data.c4[0] & 0x7) << 18) + ((d_data.c4[1] & 0x3F) << 12) + ((d_data.c4[2] & 0x3F) << 6) + (d_data.c4[3] & 0x3F);
				break;
			default:
				break;
			}
			return result;
		}

		inline CharUTF8& setUTF32(uint32_t val)
		{
			d_data.u4 = 0;
			if (val <= 0x7F)
			{
				d_data.c4[0] = val;
				d_size = 1;
			}
			else if (val <= 0x7FF)
			{
				d_data.c4[0] = ((val & 0x7C0) >> 6) | 0xC0;
				d_data.c4[1] = (val & 0x3F) | 0x80;
				d_size = 2;
			}
			else if (val <= 0xFFFF)
			{
				d_data.c4[0] = ((val & 0xF000) >> 12) | 0xE0;
				d_data.c4[1] = ((val & 0xFC0) >> 6) | 0x80;
				d_data.c4[2] = (val & 0x3F) | 0x80;
				d_size = 3;
			}
			else
			{
				d_data.c4[0] = ((val & 0xFC0000) >> 18) | 0xF0;
				d_data.c4[1] = ((val & 0x3F000) >> 12) | 0x80;
				d_data.c4[2] = ((val & 0xFC0) >> 6) | 0x80;
				d_data.c4[3] = (val & 0x3F) | 0x80;
				d_size = 4;
			}

			return *this;
		}

		static CharUTF8 fromUTF32(uint32_t val)
		{
			CharUTF8 result;
			return result.setUTF32(val);
		}

		bool toLower();
		bool toUpper();

		bool isLower() const;
		bool isUpper() const;

		StrUTF8& decompose(StrUTF8&) const;
	};

    inline std::ostream& operator <<( std::ostream& fp, const CharUTF8& c )
        { return fp << (const char*) (c); }


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
        /// given offset in bytes returns the glyph containing this offset
        size_t getGlyphFromOffset( size_t o ) const
        {
            auto i = std::lower_bound( m_positions.begin(), m_positions.end(), o );
            if( i == m_positions.end() ) 
                return ( m_positions.size() ? m_positions.back() : 0 );
            else 
                return (i-m_positions.begin());
        }
        const char* getBufEnd() const { return &(m_buf.back()); }

        const char* getGlyphStart( size_t g ) const
            { return &m_buf[ m_positions[g] ]; }
        const char* getGlyphEnd( size_t g ) const
            { return ( (g+1)< m_positions.size() ? &(m_buf[ m_positions[g+1]]): ( (&(m_buf[0])) +m_buf.size()) ); }
            
		std::vector<CharUTF8> getChars() const
		{
			std::vector<CharUTF8> res;
			res.reserve(size());
			for (const auto& c : *this)
				res.push_back(c);
			return res;
		}
        
        // substring from len glyphs starting with i-th glyph 
        std::string getSubstring( size_t i, size_t len ) const
        {
            if( i< m_positions.size() ) {
                size_t i_end = i+len;
                if( i_end >= m_positions.size() )
                    i_end = m_positions.size()-1;

                if( i_end > i ) { 
                    const char* b = getGlyphStart(i);
                    size_t buf_len = (m_positions[i_end]-m_positions[i]);
                    return( std::string(b, buf_len) );
                }
            } 
            return std::string();
        }
        inline static size_t glyphCount( const char* s, const char* s_end = 0 )
        {
		    size_t numGlyphs = 0;
            if( !s_end ) 
                s_end = s+strlen(s);

		    for(;s< s_end; ++numGlyphs) 
                s+= CharUTF8(s).size();

            return numGlyphs;
        }
        inline StrUTF8& assign( const char*s, const char* s_end=0 )
        {
            m_buf.clear();
            m_positions.clear();
		    const char *begin = s;
		    size_t lastPos = 0;
            if( !s_end ) 
                s_end = s+strlen(s);

		    while(s< s_end && *s) {
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
        // assigns o range of glyphs between fromGlyph,toGLyph (includes both from anfd to)
        StrUTF8& assign( const StrUTF8& o, size_t fromGlyph, size_t toGlyph ) 
            { return assign( o.getGlyphStart(fromGlyph), o.getGlyphEnd(toGlyph) ); }

		StrUTF8() {appendZero ();}

		explicit StrUTF8(const char*s)
            { assign(s); }

		StrUTF8(const char*s, size_t s_sz )
            { assign(s,s+s_sz); }

		template<typename Iter>
		StrUTF8 (Iter begin, Iter end)
		{
            for( Iter i = begin; i!= end; ++i ) addSymbol(*i);
			appendZero ();
		}

		inline bool operator==(const StrUTF8& other) const
		{
			return m_buf == other.m_buf &&
					m_positions == other.m_positions;
		}

		const char*     c_str() const           { return &m_buf[0]; }
        operator const char* () { return c_str(); }

		inline bool empty() const { return m_positions.size() == 1; }
		inline size_t size() const            { return (m_positions.size()-1); }
		inline size_t length() const          { return size(); }
		inline size_t getGlyphCount() const   { return size(); }
		inline size_t bytesCount() const      { return m_buf.size(); }

		inline void clear() {
            m_buf.clear();
            m_positions.clear();
            appendZero();
        }

		inline CharUTF8 operator[] (size_t glyphNum) const { return getGlyph(glyphNum); }

		inline CharUTF8 getGlyph(size_t glyphNum) const
		{
			const size_t pos = m_positions[glyphNum];
			return CharUTF8(&m_buf[pos],m_positions[glyphNum+1]-m_positions[glyphNum]);
		}
        size_t getGlyphSize(size_t g) const { 
            size_t g1 = g+1;
            return( g1 < m_positions.size() ? (m_positions[g1] - m_positions[g]) : 0 );
        }

		inline void setGlyph (size_t glyphNum, const CharUTF8& glyph)
		{
            if( glyphNum >= m_positions.size() ) 
                return;
            size_t readjustFrom = glyphNum+1;
            size_t oldGlypSize = getGlyphSize(glyphNum);

            // char* glyphDest = &(m_buf[m_positions[glyphNum]]);

            if( readjustFrom < m_positions.size() ) {
                if( glyph.size()> oldGlypSize ) {       // new glyph is larger than the old one
                    size_t adjustBy = glyph.size()-oldGlypSize;
                    size_t fromByte = m_positions[readjustFrom], moveSz = m_buf.size()-fromByte;
                    if( m_buf.capacity() < m_buf.size() + adjustBy ) 
                        m_buf.resize( m_buf.size() + adjustBy );
                    char* fromBuf = &( m_buf[fromByte] ), *toBuf   =fromBuf+adjustBy;
                    memmove( toBuf, fromBuf, moveSz );
                    // recomputing 
                    for( size_t i = glyphNum+1;i< m_positions.size();++i ) m_positions[i]+= adjustBy;
                } else 
                if( glyph.size()< oldGlypSize ) {     // new glyph is smaller than the current (old)
                    size_t adjustBy = oldGlypSize-glyph.size();
                    size_t fromByte = m_positions[readjustFrom], moveSz = m_buf.size()-fromByte;
                    char* fromBuf = &(m_buf[fromByte]), *toBuf= fromBuf+adjustBy;
                    memmove( toBuf, fromBuf, moveSz );
                    for( size_t i = glyphNum+1;i< m_positions.size();++i ) m_positions[i]-= adjustBy;
                } 
            }
            /// at this point m_positions[glyphNum] is ready to receive the token 
            glyph.copyToBufNoNull( &(m_buf[m_positions[glyphNum]]) );
		}

		inline void push_back (const CharUTF8& g)
		{
            m_buf.resize( m_buf.size() + g.size() );
            char* buf = &(m_buf[ m_positions.empty() ? 0 : m_positions.back() ]);
            m_positions.push_back( m_buf.size()-1);
            g.copyToBufNoNull( buf );
		}
        StrUTF8& append( const CharUTF8& o ) 
            { push_back(o); return *this; }
        StrUTF8& append( const StrUTF8& o ) 
        {
            m_buf.resize( o.m_buf.size() + m_buf.size() -1 );
            size_t offset = m_positions.back();

            m_positions.reserve( m_positions.size() + o.m_positions.size()-1 );
            m_positions.pop_back();
            for( std::vector<size_t>::const_iterator p = o.m_positions.begin(); p!= o.m_positions.end(); ++p ) 
                m_positions.push_back(*p+offset);
            memcpy( &(m_buf[ offset ]), &(o.m_buf[0]), o.m_buf.size() );
            return *this;
        }

		void swap (size_t x, size_t y)
        { 
            if( x == y ) return;
            else if( x> y ) std::swap(x,y);
            /// guaranteed x< y
            size_t y1 = y+1;
            if( y1 >= m_positions.size() ) 
                return; 

            size_t x_pos = m_positions[x], y_pos= m_positions[y], y_end_pos = m_positions[y1];
            char* buf=  &(m_buf[x_pos]);
            CharUTF8 xg(buf,&(m_buf[y_pos])), 
                     yg(&(m_buf[y_pos]), &(m_buf[y_end_pos]));
            if( x+1 == y ) { // swapping adjacent ones
                yg.copyToBufNoNull( buf );
                m_positions[y] = x_pos + yg.size();
                xg.copyToBufNoNull( buf + yg.size() );
            } else {         // swapping remote ones 
                if( xg.size() != yg.size() ) {
                    size_t x1=x+1,recomputeFrom = x1, recomputeTo = y1;
                    if( yg.size() > xg.size() ) {
                        size_t shift =  (yg.size()- xg.size());
                        for( size_t i= recomputeFrom; i!= recomputeTo; ++i ) m_positions[i] += shift;
                    } else  {
                        size_t shift =  (xg.size()- yg.size());
                        for( size_t i= recomputeFrom; i!= recomputeTo; ++i ) m_positions[i] -= shift;
                    }
                    yg.copyToBufNoNull(&(m_buf[m_positions[x]]));
                    xg.copyToBufNoNull(&(m_buf[m_positions[y]]));
                } else {
                    yg.copyToBufNoNull(&(m_buf[m_positions[x]]));
                    xg.copyToBufNoNull(&(m_buf[m_positions[y]]));
                }
            }
        }

		inline std::vector<uint32_t>&  toUTF32( std::vector<uint32_t>& result ) const
		{
            size_t this_size = size();
			result.reserve(this_size);
			for (size_t i = 0; i < this_size; ++i)
				result.push_back(getGlyph(i).toUTF32());
			return result;
		}

		inline StrUTF8& setUTF32(const std::vector<uint32_t>& ucs)
		{
			clear();
			for (std::vector<uint32_t>::const_iterator i = ucs.begin(), end = ucs.end(); i != end; ++i)
				append(CharUTF8::fromUTF32(*i));
			return *this;
		}

		/** Converts the string to lowercase and returns true if the string has
		 * been actually modified (that is, if there were uppercase chars).
		 */
		bool toLower();
		/** Converts the string to uppercase and returns true if the string has
		 * been actually modified (that is, if there were lowercase chars).
		 */
		bool toUpper();

		/** Checks if the string contains glyphs for which uppercase
		 * equivalents do exist. For a string like "$%#—" it will return false.
		 */
		bool hasLower() const
        {
            for (size_t i = 0, sz = size(); i < sz; ++i)
                if (getGlyph(i).isLower())
                    return true;
            return false;
        }
		/** Checks if the string contains glyphs for which lowercase
		 * equivalents do exist. For a string like "$%#—" it will return false.
		 */
		bool hasUpper() const
        {
            for (size_t i = 0, sz = size(); i < sz; ++i)
                if (getGlyph(i).isUpper())
                    return true;
            return false;
        }
		/** This function doesn't parse the whole string and returns true as
		 * soon as it founds first lowercase character (if any). So it's better
		 * to use this function instead of constructing a temporary like this:
		 * ay::StrUTF8(s, size)::hasLower().
		 */
		static bool hasLower(const char *s, size_t size)
        {
            const char* s_end = s + size;
		    while(s < s_end ) {
			    const CharUTF8 ch(s);
			    if (ch.isLower())
				    return true;
    
			    s += ch.size();
		    }
		    return false;
        }
		/** This function is different from the non-static version in the same
		 * way as hasLower is.
		 */
		static bool hasUpper(const char *s, size_t size)
        {
            const char* s_end = s+size;
		    while(s < s_end)
		    {
			    const CharUTF8 ch(s);
			    if (ch.isUpper())
				    return true;
    
			    s += ch.size();
		    }
		    return false;
        }

		bool normalize();

        struct const_iterator {
            const StrUTF8 *m_str;
            size_t   m_pos;

            const_iterator( const StrUTF8& str, size_t p ) : m_str(&str), m_pos(p) {}

            const_iterator& operator++() { ++m_pos; return *this; }
            const_iterator& operator--() { --m_pos; return *this; }
            const_iterator& operator+=(ptrdiff_t diff) { m_pos += diff; return *this; }
            const_iterator operator +( int i ) const { return( const_iterator(*m_str, m_pos+i) ); }
            const_iterator operator -( int i ) const { return (*this + (-i)); }
            ptrdiff_t operator -( const const_iterator& o ) const { return( m_pos-o.m_pos); }

            bool operator==(const const_iterator& c) const { return c.m_pos == m_pos; }
            bool operator!=(const const_iterator& c) const { return !(*this == c); }
            bool operator<(const const_iterator& c) const { return m_pos < c.m_pos; }
            bool operator<=(const const_iterator& c) const { return m_pos <= c.m_pos; }
            bool operator>(const const_iterator& c) const { return m_pos > c.m_pos; }
            bool operator>=(const const_iterator& c) const { return m_pos >= c.m_pos; }
			inline CharUTF8 operator* () const
                { return m_str->getGlyph (m_pos); }
        };
        const_iterator begin() const { return const_iterator(*this,0); }
        const_iterator end() const { return const_iterator(*this,size()); }
	};

int unicode_normalize_punctuation( std::string& outStr, const char* srcStr, size_t srcStr_sz ) ;
int unicode_normalize_punctuation( std::string& qstr ) ;
} // ay namespace

namespace std
{
	template<>
	struct iterator_traits<ay::StrUTF8::const_iterator>
	{
		typedef random_access_iterator_tag	iterator_category;
		typedef ay::CharUTF8				value_type;
		typedef ptrdiff_t					difference_type;
		typedef const ay::CharUTF8*			pointer;
		typedef const ay::CharUTF8&			reference;
	};
}
