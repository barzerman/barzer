/*
 * barzer_en_date_util.cpp
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */

#include <lg_en/barzer_en_date_util.h>

namespace barzer {
    namespace en {
    const uint8_t lookupMonth(const char* mname) {
        if (strlen(mname) < 3) return 0;
        switch (*mname) {
            case 'a': 
                switch(mname[1]) {
                    case 'u': return 8; break;          //august
                    case 'p': return 4; break;          //april
                    default: return 0;
                } break;
            case 'd': return 12; break;                 //december
            case 'f': return 2; break;                  //february
            case 'j': 
                switch (mname[1]) {
                    case 'a': return 1; break;          //january
                    case 'u': 
                        switch(mname[2]) {
                            case 'n': return 6; break;  //june
                            case 'l': return 7; break;  //july
                            default: return 0;
                        } break;
                        default: return 0;
                } break;                   
            case 'm': 
                switch (mname[2]) {
                    case 'y': return 5; break;          //may
                    case 'r': return 3; break;          //march
                    default: return 0;
                } break;
            case 'n': return 11; break;                 //november
            case 'o': return 10; break;                 //october
            case 's': return 9; break;                  //september
            default: return 0;
        }
    }
    const uint8_t lookupWeekday(const char* wdname)
    {
        if (strlen(wdname) < 2) return 0;
        switch (*wdname) {
            case 'm': return 1; break;          //monday
            case 't': 
                switch(wdname[1]) {
                    case 'u': return 2; break;  //tuesday
                    case 'h': return 4; break;  //thursday
                    default: return 0;
                } break;
            case 'w': return 3; break;          //wednesday
            case 'f': return 5; break;          //friday
            case 's':
                switch(wdname[1]){
                    case 'a': return 6; break;  //saturday
                    case 'u': return 7; break;  //sunday
                    default: return 0;
                } break;
            default: return 0;
        }
    }
}
}
