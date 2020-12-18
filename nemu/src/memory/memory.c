#include "common.h"
//define some attributes
#define BLOCK_SIZE 64 //8byte
#define L1_SIZE  64*1024
#define SET_SIZE 8                            //8-way set associate
#define LINE_SZIE 1024/8 

/*cache line :   ******19*******|| ****7****||****6****
                                               tag                    index          offset   */
typedef struct 
{
	bool valid;
	 uint32_t tag;
     uint8_t  data[BLOCK_SIZE];
}Cache;
Cache L1[SET_SIZE][LINE_SZIE];  /*cache1 1024            */

void init_cache(){
	int i,j;
	for ( i = 0; i < SET_SIZE; i++)
	{
		for ( j = 0; i < LINE_SZIE; j++)
		{
			 L1[i][j].valid = false;
		     memset (L1[i][j].data ,0 ,BLOCK_SIZE);
		}   
	}
}

/*uint32_t cache_read(hwaddr_t ,size_t){
  
}*/
uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	/* 0u : 0000 0000 0000 0000
	 ~0u : 1111 1111 1111 1111   */
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

