#pragma once
#include <stdint.h>
#include <utility>

/// no types dependent on anything other than std / stdint  are allowed
namespace barzer {

typedef  std::pair< uint32_t , uint32_t > UniqueTrieId;

inline bool operator<(const UniqueTrieId& l, const UniqueTrieId& r)
    { return( l.first < r.first ? true : ( r.first < l.first ? false : (l.second < r.second) )); }


} // namespace barzer 
