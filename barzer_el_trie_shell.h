/*
 * barzer_el_trie_shell.h
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */

#ifndef BARZER_EL_TRIE_SHELL_H_
#define BARZER_EL_TRIE_SHELL_H_

#include <ay/ay_util_char.h>
#include <barzer_shell.h>
#include <barzer_universe.h>
#include "barzer_el_trie_walker.h"
#include "barzer_el_trie_shell.h"

namespace barzer {

typedef ay::char_cp char_cp;
typedef int (*TrieShellFun)( BarzerShell*, char_cp cmd, std::istream& in );


int bshtrie_process( BarzerShell* shell, char_cp cmd, std::istream& in );

}

#endif /* BARZER_EL_TRIE_SHELL_H_ */
