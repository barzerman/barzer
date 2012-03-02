#ifndef BARZER_ARCG_H
#define BARZER_ARCG_H

#ifdef __MINGW32__
#include <arch/barzer_windows.h>
#endif

#ifdef __linux__
#define LOCALTIME_R(x, y) localtime_r(x, y)
#endif
#endif //BARZER_ARCG_H