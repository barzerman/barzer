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
        char c0 = ( mname[0] ? tolower(mname[0]) : 0 );
        char c1 = ( c0 ? tolower(mname[1]) : 0 );
        char c2 = ( c1 ? tolower(mname[2]) : 0 );        
        switch (c0) {
            case 'a': 
                switch(c1) {
                    case 'u': return 8; break;          //august
                    case 'p': return 4; break;          //april
                    default: return 0;
                } break;
            case 'd': return 12; break;                 //december
            case 'f': return 2; break;                  //february
            case 'j': 
                switch (c1) {
                    case 'a': return 1; break;          //january
                    case 'u': 
                        switch(c2) {
                            case 'n': return 6; break;  //june
                            case 'l': return 7; break;  //july
                            default: return 0;
                        } break;
                        default: return 0;
                } break;                   
            case 'm': 
                switch (c2) {
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
        char c0 = ( wdname[0] ? tolower(wdname[0]) : 0 );
        char c1 = ( c0 ? tolower(wdname[1]) : 0 );        
        switch (c0) {
            case 'm': return 1; break;          //monday
            case 't': 
                switch(c1) {
                    case 'u': return 2; break;  //tuesday
                    case 'h': return 4; break;  //thursday
                    default: return 0;
                } break;
            case 'w': return 3; break;          //wednesday
            case 'f': return 5; break;          //friday
            case 's':
                switch(c1){
                    case 'a': return 6; break;  //saturday
                    case 'u': return 7; break;  //sunday
                    default: return 0;
                } break;
            default: return 0;
        }
    }
}
}
