#include "trap.h"
#define Cachesize (64*1024*16)
int main(){
    int a[Cachesize/4];
    int  i;
    int k;
    for ( i = 0; i < (Cachesize/4); i++)
    {
        k = a[i];
    }
    return 0;
}


