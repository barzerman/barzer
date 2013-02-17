
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_el_trie_shell.h
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#pragma once 

#include <ay/ay_util_char.h>
#include <barzer_shell.h>
#include <barzer_el_trie_walker.h>
#include <barzer_el_trie_shell.h>

namespace barzer {

typedef ay::char_cp char_cp;
typedef int (*TrieShellFun)( BarzerShell*, std::istream& in );


int bshtrie_process( BarzerShell* shell, std::istream& in );

} // namespace barzer
