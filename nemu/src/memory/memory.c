#include "common.h"
#include "cpu/reg.h"
#include <stdlib.h>


uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

uint32_t read_cache(hwaddr_t addr, size_t len, bool *flag);
bool write_cahce(hwaddr_t addr, size_t len, uint32_t data);


/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len)
 {
 	bool flag;
 	uint32_t d1, d2;
 	uint32_t up = (addr & ~((1 << cpu.cache1.b) - 1)) + (1 << cpu.cache1.b);
        //printf("Read\n");
        //printf("addr:0x%x,up:0x%x\n",addr,up);
 	if (addr + len > up)
 	{
                //printf("Hi\n");
 		d1 = read_cache(addr, up - addr, &flag);
                //printf("Hi\n");
 		d2 = read_cache(up, addr + len - up, &flag);
                //printf("Hi\n");
 		return d1 + (d2 << 16);
 	}
 	return read_cache(addr, len, &flag);
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data)
 {
 	bool f;
 	uint32_t up = (addr & ~((1 << cpu.cache1.b) - 1)) + (1 << cpu.cache1.b);
        //printf("Read\n");
 	if (addr + len > up)
 	{
 		f = write_cahce(addr, up - addr, data);
 		if (!f)
 		{
 			dram_write(addr, up - addr, data);
 		}
 		f = write_cahce(up, addr + len - up, data);
 		if (!f)
 		{
 			dram_write(addr, addr + len - up, data);
 		}
 	}
 	f = write_cahce(addr, len, data);
 	if (!f)
 	{
 		dram_write(addr, len, data);
 	}
 	//dram_write(addr, len, data);
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


uint32_t read_cache(hwaddr_t addr, size_t len, bool *flag)
 {
 	uint32_t index, tag, off, E;
 	struct set *set;
 	int i, j;
 	index = (addr >> cpu.cache1.b) & ((1 << cpu.cache1.s) - 1);
 	tag = addr >> (cpu.cache1.s + cpu.cache1.b);
 	off = addr & ((1 << cpu.cache1.b) - 1);
 	E = cpu.cache1.E;
 	set = &cpu.cache1.sets[index];

 	for (i = 0; i < E && set->blocks[i].valid == true; i++)
 	{
 		if (set->blocks[i].tag == tag)
 		{
 			*flag = true;
 			if (len == 4)
 				return *((uint32_t *)(set->blocks[i].buf + off));
 			if (len == 2)
 				return *((uint16_t *)(set->blocks[i].buf + off));
 			if (len == 1)
 				return *((uint8_t *)(set->blocks[i].buf + off));
 		}
 	}
 	*flag = false;
 	if (i < E)
 	{
 		set->blocks[i].valid = true;
 		set->blocks[i].tag = tag;
 		for (j = 0; j < (1 << cpu.cache1.b); j++)
 		{
                        //printf("%d\n",j);
 			set->blocks[i].buf[j] = dram_read(addr - off + j, 1) & 255;
 		}
 	}
 	else
 	{
 		i = rand() % E;
 		set->blocks[i].tag = tag;
 		for (j = 0; j < (1 << cpu.cache1.b); j++)
 		{
 			set->blocks[i].buf[j] = dram_read(addr - off + j, 1) & 255;
 		}
 	}
 	if (len == 4)
 		return *((uint32_t *)(set->blocks[i].buf + off));
 	if (len == 2)
 		return *((uint16_t *)(set->blocks[i].buf + off));
 	if (len == 1)
 		return *((uint8_t *)(set->blocks[i].buf + off));
 	return 0;
 }
 bool write_cahce(hwaddr_t addr, size_t len, uint32_t data)
 {
 	uint32_t index, tag, off, E;
 	struct set *set;
 	int i;
 	index = (addr >> cpu.cache1.b) & ((1 << cpu.cache1.s) - 1);
 	tag = addr >> (cpu.cache1.s + cpu.cache1.b);
 	off = addr & ((1 << cpu.cache1.b) - 1);
 	E = cpu.cache1.E;
 	set = &cpu.cache1.sets[index];

 	for (i = 0; i < E && set->blocks[i].valid == true; i++)
 	{
 		if (set->blocks[i].tag == tag)
 		{
 			if (len == 4)
 				*((uint32_t *)(set->blocks[i].buf + off)) = data;
 			if (len == 2)
 				*((uint16_t *)(set->blocks[i].buf + off)) = data;
 			if (len == 1)
 				*((uint8_t *)(set->blocks[i].buf + off)) = data;
 			dram_write(addr, len, data);
 			return true;
 		}
 	}
 	return false;
 } 

