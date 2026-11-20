#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* this is a weak printf placeholder */
/* so we can change its behavior */
__attribute__((weak)) int my_printf(const char *__restrict__ str, ...) 
{
    int result;
    va_list params;
    va_start(params, str);
    result = vprintf(str, params);
    va_end(params);
    return result;    
}

