#ifndef BARZER_ARCG_H
#define BARZER_ARCG_H

#ifdef _WINDOWS_
    #include <arch/barzer_windows.h>
#else
    #include <sys/time.h>
    #define LOCALTIME_R(x, y) localtime_r(x, y)
#endif
#endif //BARZER_ARCG_H
