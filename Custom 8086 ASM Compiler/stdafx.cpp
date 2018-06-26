#include "stdafx.h"


bool IsAlphaOnly(const char* s)
{
    while (*s) 
    {
        if ((isalpha(*s++) == 0))   return false;
    }

    return true;
}

// either binary,decimal or hex
bool IsNumbersOnly(const char *s)
{
    while (*s) {
        if ((isxdigit(*s) == 0)
            && (isdigit(*s) == 0)
            && (*s != '\n')
            && (*s != '\t')
            && (*s != ' ')
            && (*s != 'x')
            && (*s != 'b')) return false;

        s++;
    }
    return true;
}
