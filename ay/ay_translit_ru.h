#pragma once

#include <vector>
#include <string>

namespace ay
{
namespace tl
{
	typedef std::pair<std::string, std::string> TLException_t;
	typedef std::vector<TLException_t> TLExceptionList_t;
	void ru2en(const char *russian, size_t len, std::string& english, const TLExceptionList_t& = TLExceptionList_t());
	void en2ru(const char *english, size_t len, std::string& russian, const TLExceptionList_t& = TLExceptionList_t());
}
}
