#include "trap.h"
#define Cachesize (128*128*1024)  
int main(){
    int a[Cachesize/4];
    int  i;
    int k = 0;
    /*for ( i = 0; i < (Cachesize/4); i++)
    {
        a[i] =k ;
    }*/
    for ( i = 0; i <(Cachesize/4); i = i + 10000)
    {
        a[i]=k;
    }
    
    return 0;
}


