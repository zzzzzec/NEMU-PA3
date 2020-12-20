#include "trap.h"
#define Cachesize (64*1024*2)
int main(){
    int a[Cachesize/4];
    int  i;
    int k = 0;
    for ( i = 0; i < (Cachesize/4); i++)
    {
        a[i] =k ;
    }
    return 0;
}


