#ifndef BARZER_SHELL_H
#define BARZER_SHELL_H

#include <ay/ay_shell.h>
#include <iostream>
namespace barzer {

struct BarzerShell : public ay::Shell {

virtual int init();
};

}

#endif
