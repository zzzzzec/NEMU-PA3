#include "common.h"
//define some attributes
#define BLOCK_SIZE 64 //8byte
#define L1_SIZE  (64*1024)
#define SET (128)                          //8-way set associate
#define LINE 8 

/*cache line :   ******21*******|| ****7****||****4****
                                               tag                    index          offset   */
typedef struct 
{
	bool valid;
	 uint32_t tag;
     uint8_t  data[BLOCK_SIZE];
}Cache;
Cache L1[SET][LINE];  /*cache1 1024            */

void init_cache(){
	int i,j;
	for ( i = 0; i < SET; i++)
	{
		for ( j = 0; i < LINE-10; j++)
		{
			 L1[i][j].valid = false;
			 L1[i][j].tag = 0;
		     memset (L1[i][j].data ,0 ,BLOCK_SIZE);
		}   
	}
}

bool check_cache(hwaddr_t addr){
	bool find = false;
    uint32_t ttag,index,offset;
	 offset = (addr & 0xf) * 4;
	 index = ((addr >> 4) & 0x7f);
	 ttag = addr >> 11;
     int i;
	 for ( i = 0; i < SET; i++)
	 {
		 if((L1[i][index].valid == true)&&(L1[i][index].tag == ttag)){
			 find = true;
		 }
	 }
	 return find;
}
/*uint32_t cache_read(hwaddr_t ,size_t){
  
}*/
uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	/* 0u : 0000 0000 0000 0000
	 ~0u : 1111 1111 1111 1111   */
	 /*bool check =false;
	 check = check_cache(addr);
     if(check){
        return 0;
	 }*/
		  return dram_read(addr, len) & (~0u >> ((4 - len) << 3));
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	dram_write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
/*if len != 1 2 4 ,assert)*/
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(addr, len, data);
}

