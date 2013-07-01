
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <string> 
/// implementation of stemmers / normalizers for Russian language 
/// utf8 strings 
namespace barzer {
class Russian_StemmerSettings;

namespace Russian {

// normalizer - tries to preserve part of speech 
// 
bool normalize( std::string& out, const char* s, size_t* len, const Russian_StemmerSettings* stset = 0 ) ;
} // namespace Russian

} // namespace barzer
