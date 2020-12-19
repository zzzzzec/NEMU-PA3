#include "common.h"
//define some attributes
#define BLOCK_SIZE 64 //8byte
#define L1_SIZE (64 * 1024)
#define SET (128) //8-way set associate
#define LINE 8
int number = 0;

/*cache line :   ******21*******|| ****7****||****4****
                                               tag                    index          offset   */
typedef struct
{
	bool valid;
	uint32_t tag;
	uint8_t data[BLOCK_SIZE];
} Cache;
Cache L1[SET][LINE]; /*cache1 1024            */

void init_cache()
{
	int i, j;
	for (i = 0; i < SET; i++)
	{
		for (j = 0; j < LINE; j++)
		{
			L1[i][j].valid = false;
			L1[i][j].tag = 0;
			memset(L1[i][j].data, 0, BLOCK_SIZE);
		}
	}
}

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

int get_num()
{
    number ++;
	number = number % 8;
	return number;
}

/* Memory accessing interfaces */
/*copy 64B from ram to cache*/
void M2C(hwaddr_t addr,uint32_t set ,int line){
            uint32_t tem[16];
			int k=0;
			for ( k = 0; k < 16; k++)
			{
				tem[k] = dram_read(addr, 4);
				/*printf("0x%08x ",dram_read(addr , 4));*/
				addr += 4;
			}
				for ( k = 0; k < 16; k++)
			{
				printf("0x%08x ",tem[k]);
			}
			memcpy(tem , L1[set][line].data , 64);	  
}

uint32_t hwaddr_read(hwaddr_t addr, size_t len)
{
	/* 0u : 0000 0000 0000 0000
	 ~0u : 1111 1111 1111 1111   */
	bool find = false;
	uint32_t set, ttag, offset;
	set = (addr >> 4) & (0x7f);
	ttag = (addr >> 11);
	offset = (addr & 0xf);
	printf("set:0x%07x \ntag:0x%21x \noffset:0x%04x \n"
		,set,ttag,offset);
	int i;
	for (i = 0; i < LINE; i++)
	{
		if (L1[set][i].valid == true && L1[set][i].tag == ttag)
		{
			find = true;
			break;
		}
	}
	if (find == true)
	{
		printf("TRUE \n");
		uint32_t result[2];
		memcpy(result, L1[set][i].data + (4 * offset), 4);
		return result[0] & (~0u >> ((4 - len) << 3));
	}
	else
	{
		printf("FALSE\n");
		bool empty;
		int j = 0;
		for (j = 0; j < LINE; j++)
		{
			if(L1[set][j].valid == 0)
			{
				empty = true;
				break;
			}
		}
		if(empty)
		{
			printf("EMPTY \n");
		   M2C(addr , set,j);
		}
		else /*cache full*/
		{
			printf("FULL \n");
              int p =get_num();
			  M2C(addr,set ,p);
		}
		uint32_t result[2];
		memcpy(result, L1[set][i].data + (4 * offset), 4);
		/*return result[0] & (~0u >> ((4 - len) << 3));*/
		return dram_read(addr,len) & (~0u >> ((4 - len) << 3));
	}
}


void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data)
{
	dram_write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len)
{
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data)
{
	hwaddr_write(addr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len)
{
#ifdef DEBUG
	/*if len != 1 2 4 ,assert)*/
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data)
{
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(addr, len, data);
}

