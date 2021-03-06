
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_ru_date_util.cpp
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */

#include <lg_ru/barzer_ru_date_util.h>

namespace barzer {
namespace ru {
    const uint8_t lookupMonth(const char* mname){
        if (strlen(mname) < 3) return 0;
        ay::Char2B_accessor m(mname);
        
        if (m("я")) return 1; else      //январь
        if (m("а")) {
            ay::Char2B_accessor mn = m.next();
            if (mn("в")) return 8; else //август
            if (mn("п")) return 4; else //апрель
            return 0; } else 
        if (m("д")) return 12; else     //декабрь
        if (m("и"))  {
            ay::Char2B_accessor  mnn = m.next().next();
            if (mnn("л")) return 7; else //июль
            if (mnn("н")) return 6; else //июнь
            return 0;} else
        if (m("м")) {
            ay::Char2B_accessor  mnn = m.next().next();
            if (mnn("й")) return 5; else//май
            if (mnn("р")) return 3; else//март
            return 0;} else
        if (m("н")) return 11; else     //ноябрь
        if (m("о")) return 10; else     //октябрь
        if (m("с")) return 9; else      //декабрь
        if (m("ф")) return 2; else      //февраль
        return 0; 
    }
 
    const uint8_t lookupWeekday(const char* wdname){
        if (strlen(wdname) < 2) return 0;
        ay::Char2B_accessor w(wdname);
        
        if (w("с")) {
            ay::Char2B_accessor wn = w.next();
            if (wn("р")) return 3; else //среда, ср
            if (wn("у")) return 6; else //суббота
            if (wn("б")) return 6; else //сб
            return 0; } else 
        if (w("в"))  {
            ay::Char2B_accessor  wn = w.next();
            if (wn("т")) return 2; else //вторник, вт
            if (wn("о")) return 7; else //воскресенье
            if (wn("с")) return 7; else //вс
            return 0;} else
        if (w("п")) {
            ay::Char2B_accessor  wn = w.next();
            if (wn("о")) return 1; else//понедельник
            if (wn("н")) return 1; else//пн
            if (wn("я")) return 5; else//пятница
            if (wn("т")) return 5; else//пт
            return 0;} else
        if (w("ч")) return 4; else     //четверг
        return 0; 
    }
}   //namespace ru
}   //namespace barzer
