/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <time.h>
//#include <sys/time.h>
//#include <sys/types.h>

#include "gamera.hpp"

#ifndef kwm07192002_utility
#define kwm07192002_utility

namespace Gamera {

  inline void byte_swap32(unsigned char *ptr) {
    register unsigned char val;
    
    val = *(ptr);
    *(ptr) = *(ptr+3);
    *(ptr+3) = val;
    
    ptr += 1;
    val = *(ptr);
    *(ptr) = *(ptr+1);
    *(ptr+1) = val;
  }
  
  class Clocker {
  public:
    void start();
    void stop();
    float seconds();
    float microseconds();
  private:
    clock_t _start;
    clock_t _end;
  };
  
  inline void Clocker::start() {
    _start = clock();
  }
  
  inline void Clocker::stop() {
    _end = clock();
  }
  
  inline float Clocker::seconds() {
    return ((float)(_end - _start) / (float)CLOCKS_PER_SEC);
  }
  
  inline float Clocker::microseconds() {
    return ((float)((_end - _start) / (float)CLOCKS_PER_SEC) * 1000000);
  }
#if 0

  /**
     Alternative to Clocker, which wraps after about 35.79 minutes
     Clocker measures CPU time, Timer measures real time.
     Clocker resolution is 10ms on Linux/Intel
     Timer resolution is about 10 microseconds
     If a finer resolution is needed checkout POSIX.4 clock_gettime()
     
     01/06/06 IF started
     6/7/01 KWM integrated into Gamera
  */

  class Timer {
  public:
    void start();
    void stop();
    float seconds();
    Timer();
  private:
    struct timeval _start;
    struct timeval _end;
    struct timezone tz; // not used
  };
  
  Timer::Timer() {
    _start.tv_sec = _start.tv_usec = 0;
    _end.tv_sec = _end.tv_usec = 0;
  }
  
  inline void Timer::start() {
    gettimeofday(&_start, &tz);
  }
  
  inline void Timer::stop() {
    gettimeofday(&_end, &tz);
  }
  
  inline float Timer::seconds() {
    return ((_end.tv_sec - _start.tv_sec) +
	    (_end.tv_usec - _start.tv_usec) / 1000000.);
  }

#endif  
}; // namespace Gamera

#endif
