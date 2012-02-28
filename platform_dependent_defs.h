#ifndef PLATFORM_DEPENDENT_DEFS_H
#define PLATFORM_DEPENDENT_DEFS_H

#if defined(_WINDOWS_) || defined(WIN32)
#define LOCALTIME_R(x, y) memcpy(y, localtime(x), sizeof(y))
#else
#define LOCALTIME_R(x, y) localtime_r(x, y)
#endif


#if defined(_MSC_VER) || defined(_WINDOWS_)
   #include <time.h>
   #if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_)
         struct timeval 
         {
            long tv_sec;
            long tv_usec;
         };
   #endif 
#else
   #include <sys/time.h>
#endif 
#if defined(_MSC_VER) || defined(_WINDOWS_)
   int gettimeofday(struct timeval* tp, void* tzp) 
   {
      DWORD t;
      t = timeGetTime();
      tp->tv_sec = t / 1000;
      tp->tv_usec = t % 1000;
      return 0;
   }
#endif


#if defined(WIN32) 

#define NOGDI		//to prevent including wingdi.h that contains #define ERRROR 0 that conflicts with enum Logger::LogLevelEnum

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define uint unsigned int
#define suseconds_t unsigned long	//is not presented in win

#undef ERROR						//needs to enum Logger::LogLevelEnum

#undef LANG_ENGLISH		//
#undef LANG_RUSSIAN		// for enum in barzer_language.h:14 
#undef LANG_SPANISH		//
#undef LANG_FRENCH		//
				
#undef DELETE			// for enum in barzer_server_request.h:59 conflicts with WinNT.h


#if defined(_MSC_VER)
#define __func__ __FUNCTION__
#endif // defined(_MSC_VER)

#endif // defined(WIN32)
#endif //PLATFORM_DEPENDENT_DEFS_H