#include "common.h"
//define some attributes
#define MAX_MEM (0x8000000 - 1)
#define BLOCK_SIZE 64 //8byte
#define L1_SIZE (64 * 1024)
#define SET (128) //8-way set associate
#define LINE 8
int number = 0;
uint64_t testtime = 0;
uint64_t hint = 0;
uint64_t miss = 0;
/*cache line :   ******19*******|| ****7****||****6****
                                               tag                    index          offset   */
typedef struct
{
	bool valid;
	uint32_t tag;
	uint8_t data[BLOCK_SIZE];
} Cache;
Cache L1[SET][LINE]; /*cache1 1024            */

typedef struct 
{
	hwaddr_t addr;
	uint32_t set;
	uint32_t ttag;
	uint32_t offset;
}addr_D;

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
addr_D divide_addr(hwaddr_t addr , addr_D addr_d){
	    addr_d.addr =addr;
        addr_d.set= (addr >> 6) & (0x7f);
		addr_d.ttag= (addr >> 13);
		addr_d.offset= (addr & 0x3f);
		return addr_d;
}
int search_cache(addr_D addr_d){
    /*find: return line number else return -1*/
     int i;
	for (i = 0; i < LINE; i++)
	{
		if (L1[addr_d.set][i].valid == true && L1[addr_d.set][i].tag == addr_d.ttag)
		{
	          return i;
		}
	}
	return -1;
}

void view_cache(hwaddr_t addr)
{
	addr_D addr_d;
	addr_d = divide_addr(addr,addr_d);
	printf("set:0x%07x \ntag:0x%19x \noffset:0x%06x \n", addr_d.set, addr_d.ttag, addr_d.offset);
	int find = search_cache(addr_d);
	if(find == -1)
	{
		printf("addr 0x%x has not be stored in cache \n",addr_d.addr);
		return;
	}
	printf("Cache Set %d    line %d \n", addr_d.set,find);
	int i;
	for (i = 0; i < 64; i++)
	{
		printf("%02x ", L1[addr_d.set][find].data[i]);
		if ((i + 1) % 16 == 0)
		{
			printf("\n");
		}
	}
	uint32_t result[2];
	memcpy(result, L1[addr_d.set][find].data + (addr_d.offset), 4);
	printf("the value of addr is : 0x%x",result[0] );
}

/* Memory accessing interfaces */
/*copy 64B from ram to cache*/
void M2C(hwaddr_t addr, uint32_t set, int line)
{
	/*we must make sure that addr can divide by 64(blocksize)*/
	addr = addr - (addr % 64);
	uint32_t tem[16];
	L1[set][line].valid = true;
	L1[set][line].tag = addr>>13;
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
	addr_D addr_d;
	addr_d = divide_addr(addr,addr_d );
	int find = search_cache(addr_d);
	if (find != -1)
	{
		//printf("Cache hit at set %d line %d!!!!!    \n",set,i);
		testtime += 2;
		hint++;
		uint32_t result[2];
		memcpy(result, L1[addr_d.set][find].data + (addr_d.offset), 4);
		/*view_cache(set, i);
		printf("0x%08x \n", result[0]);
		printf("0x%08x \n", result[0] & (~0u >> ((4 - len) << 3)));
		printf("\n \n");*/
		return result[0] & (~0u >> ((4 - len) << 3));
	}
	else
	{
		//printf("Cache miss!!!!!    \n");
		testtime += 200;
		miss++;
		if ((addr + 64) >= MAX_MEM) /*do not add this block into cache*/
		{
			//printf("EDGE!!!!!    \n");
			return dram_read(addr, len) & (~0u >> ((4 - len) << 3));
		}
        uint32_t result[2];
		bool empty;
		int j = 0;
		for (j = 0; j < LINE; j++)
		{
			if (L1[addr_d.set][j].valid == 0)
			{
				empty = true;
				break;
			}
		}
		if (empty)
		{
			//printf("NO FULL!!!!!    \n");
			M2C(addr_d.addr, addr_d.set, j);
			memcpy(result, L1[addr_d.set][j].data + (addr_d.offset), 4);
		}
		else /*cache full*/
		{
			//printf("FULL!!!!!    \n");
			int p = get_num();
			M2C(addr_d.addr, addr_d.set,p);
			memcpy(result, L1[addr_d.set][p].data + (addr_d.offset), 4);
		}
		/*view_cache(set, j);
		printf("0x%08x \n", result[0]);
		printf("0x%08x \n", result[0] & (~0u >> ((4 - len) << 3)));
		printf("\n \n");*/
		return result[0] & (~0u >> ((4 - len) << 3));
		/*return dram_read(addr,len) & (~0u >> ((4 - len) << 3));*/
	}
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data)
{ /*write through*/
	addr_D addr_d;
	addr_d = divide_addr(addr ,addr_d);
    int find = search_cache(addr_d);
	if (find != -1)
	{
		/*printf("LEN is %ld  Addr is 0x%x\n", len, addr);
		printf("data is 0x%08x \n",data);*/
		memcpy(L1[addr_d.set][find].data + addr_d.offset, &data, len);
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
