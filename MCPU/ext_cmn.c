#include "ext_cmn.h"
//#include "memwatch.h"




int strlen_r( char *str , char end)
{
    int len = 0;

    while( (*str++) != end )
    {
        len++;
    }

    return len;
}



