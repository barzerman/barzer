#ifndef BARZER_ARCG_H
#define BARZER_ARCG_H

#if defined(_WINDOWS_) || defined(WIN32)
    #include <arch/barzer_windows.h>
#else
    #include <sys/time.h>
    #define LOCALTIME_R(x, y) localtime_r(x, y)
#endif
#endif //BARZER_ARCG_H
