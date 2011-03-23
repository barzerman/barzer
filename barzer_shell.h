#ifndef BARZER_SHELL_H
#define BARZER_SHELL_H

#include <ay/ay_shell.h>
#include <iostream>
#include <ay/ay_headers.h>
namespace barzer {

struct BarzerShellContext;
struct BarzerShell : public ay::Shell {
virtual int init();
virtual ay::ShellContext* mkContext();

BarzerShellContext* getBarzerContext() ;
	
};

}

#endif
