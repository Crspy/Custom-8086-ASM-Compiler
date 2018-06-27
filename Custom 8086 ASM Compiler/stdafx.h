// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <tchar.h>
#include <iostream>
#include <ios>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <Windows.h>
#include <map>
#include <vector>

#define VERBOSE

#ifdef VERBOSE
#define logger(x) std::cout << (x) << "\n"
#else
#define logger(x) //
#endif

bool is_alpha_only(const char* s);
bool is_numbers_only(const char *s);

template<class T>
bool is_blank_line(const T* s)
{
    while (*s)
    {
        if ((isblank(*s++) == 0))   return false;
    }

    return true;
}




// reference additional headers your program requires here
