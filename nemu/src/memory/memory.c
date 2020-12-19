#include "common.h"
//define some attributes
#define MAX_MEM (0x8000000-1)
#define BLOCK_SIZE 64 //8byte
#define L1_SIZE (64 * 1024)
#define SET (128) //8-way set associate
#define LINE 8
int number = 0;

/*cache line :   ******19*******|| ****7****||****6****
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
	number++;
	number = number % 8;
	return number;
}
void view_cache(uint32_t set, uint32_t line)
{
	printf("Cache Set %d    line %d \n ",set,line);
	int i;
	for (i = 0; i < 64; i++)
	{
		printf("%02x ", L1[set][line].data[i]);
		if ((i + 1) % 16 == 0)
		{
			printf("\n");
		}
	}
}

/* Memory accessing interfaces */
/*copy 64B from ram to cache*/
void M2C(hwaddr_t addr, uint32_t set, int line)
{
	/*we must make sure that addr can divide by 64(blocksize)*/
	 addr = addr-(addr% 64 );
	uint32_t tem[16];
	uint32_t ttag = (addr >> 13);
	L1[set][line].valid = true;
	L1[set][line].tag = ttag;
	int k = 0;
		for (k = 0; k < 16; k++)
		{
			tem[k] = dram_read(addr, 4);
			addr += 4;
		}
	memcpy(L1[set][line].data, tem, 64);
}

uint32_t hwaddr_read(hwaddr_t addr, size_t len)
{
	int len1 = len;
	printf("LEN is %d  Addr is 0x%x\n", len1, addr);
	bool find = false;
	uint32_t set, ttag, offset;
	set = (addr >> 6) & (0x7f);
	ttag = (addr >> 13);
	offset = (addr & 0x3f);
	printf("set:0x%07x \ntag:0x%19x \noffset:0x%06x \n", set, ttag, offset);
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
		printf("Cache hit at set %d line %d!!!!!    \n",set,i);
		uint32_t result[2];
		memcpy(result, L1[set][i].data + (offset), 4);
		view_cache(set, i);
		printf("0x%08x \n", result[0]);
		printf("0x%08x \n", result[0] & (~0u >> ((4 - len) << 3)));
		printf("\n \n");
		return result[0] & (~0u >> ((4 - len) << 3));
	}
	else
	{
		printf("Cache miss!!!!!    \n");
		bool empty;
		int j = 0;
		for (j = 0; j < LINE; j++)
		{
			if (L1[set][j].valid == 0)
			{
				empty = true;
				break;
			}
		}
	  if ((addr + 64) >= MAX_MEM)  /*do not add this block into cache*/
	  {
		  printf("EDGE!!!!!    \n");
         return dram_read(addr,len) & (~0u >> ((4 - len) << 3));
	  }
		if (empty)
		{
			printf("NO FULL!!!!!    \n");
			M2C(addr, set, j);
		}
		else /*cache full*/
		{
			printf("FULL!!!!!    \n");
			int p = get_num();
			M2C(addr, set, p);
		}
		uint32_t result[2];
		memcpy(result, L1[set][j].data + (offset), 4);
		view_cache(set, j);
		printf("0x%08x \n", result[0]);
		printf("0x%08x \n", result[0] & (~0u >> ((4 - len) << 3)));
		printf("\n \n");
		return result[0] & (~0u >> ((4 - len) << 3));
		/*return dram_read(addr,len) & (~0u >> ((4 - len) << 3));*/
	}
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data)
{   /*write through*/
    uint32_t set, ttag, offset;
	set = (addr >> 6) & (0x7f);
	ttag = (addr >> 13);
	offset = (addr & 0x3f);
	bool find;
	int i;
	for (i = 0; i < LINE; i++)
	{
		if (L1[set][i].valid == true && L1[set][i].tag == ttag)
		{
			find = true;
			break;
		}
	}
	if(find){ 
		printf("LEN is %ld  Addr is 0x%x\n", len, addr);
		printf("data is 0x%d04x \n",data);
	    memcpy(L1[set][i].data +offset , &data ,len);
		assert(find != true);
	}
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
