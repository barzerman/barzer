/**********************************************************************
 * eBSON11 — BSON encoder in C++11.
 *
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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ebson11
{
/** @brief Implements an incrementable string representation of 20 digit decimals.
 *
 * 20 digits since max int64 is 18,446,744,073,709,551,615.
 *
 * The intended use is this: you start with an index and increment it by 1.
 * 32bit unsigned is practically enough .
 */
class StrRepDecimal
{
	char d_buf[21];
	char *d_first;
public:
	StrRepDecimal()
	: d_first(d_buf + sizeof(d_buf) - 2)
	{
		*d_first = '0';
		d_first[1] = 0;
	}

	explicit StrRepDecimal(uint32_t i)
	{
		*this = i;
	}

	StrRepDecimal(const StrRepDecimal& other)
	{
		std::memcpy(d_buf, other.d_buf, sizeof(d_buf));
		d_first = d_buf + (other.d_first - other.d_buf);
	}

	StrRepDecimal& operator=(uint32_t i)
	{
		std::snprintf(d_buf, sizeof(d_buf), "%u", i);
		return *this;
	}

    inline void increment()
	{
		for(char* p = d_buf + sizeof(d_buf) - 2; p >= d_first; --p)
		{
			if (*p >= '9')
				*p = '0';
			else
			{
				++(*p);
				return;
			}
		}

		if(d_first > d_buf)
			*(--d_first) = '1';
	}

	const char* operator++()
	{
		increment();
		return c_str();
	}

	const char* c_str() const { return d_first; }
};

}
