#ifndef BARZER_RU_STEMMER_H
#define BARZER_RU_STEMMER_H

#include <string> 
/// implementation of stemmers / normalizers for Russian language 
/// utf8 strings 
namespace barzer {

namespace Russian {

// normalizer - tries to preserve part of speech 
// 
bool normalize( std::string& out, const char* s, size_t* len=0 ) ;
} // namespace Russian

} // namespace barzer
#endif // BARZER_RU_STEMMER_H
