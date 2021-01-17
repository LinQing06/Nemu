#include "common.h"
#include  "cpu/reg.h"
#include <stdlib.h>
#include "burst.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);
uint32_t cache_read_l1(hwaddr_t, size_t);
void cache_write_l1(hwaddr_t, size_t, uint32_t);
uint32_t cache_read_l2(hwaddr_t, size_t);
void cache_write_l2(hwaddr_t, size_t, uint32_t);
extern int is_mmio(hwaddr_t);
extern uint32_t mmio_read(hwaddr_t,size_t,int);
extern void mmio_write(hwaddr_t,size_t,uint32_t,int);
lnaddr_t seg_translate(swaddr_t, size_t, uint8_t);
hwaddr_t page_translate(lnaddr_t);
CPU_state cpu;
extern uint8_t current_sreg;

/* Memory accessing interfaces */



uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	if(is_mmio(addr)!=-1)
		return mmio_read(addr,len,is_mmio(addr));
	else
		return cache_read_l1(addr, len) & (~0u >> ((4 - len) << 3));
	//return dram_read(addr, len) & (~0u >> ((4 - len) << 3));
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	if(is_mmio(addr)!=-1)
	{
		mmio_write(addr,len,data,is_mmio(addr));
	}
	else{
		cache_write_l1(addr, len, data);
		cache_write_l2(addr, len, data);
	}
	//dram_write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
    #endif
	size_t max_len = ((~addr) & 0xfff) + 1;
    	if (len > max_len) 
    	{
        		uint32_t low = lnaddr_read(addr, max_len);
        		uint32_t high = lnaddr_read(addr + max_len, len - max_len);
        		return (high << (max_len << 3)) | low;
    	}
	hwaddr_t hwaddr = page_translate(addr);
	return hwaddr_read(hwaddr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
	#endif
	size_t max_len = ((~addr) & 0xfff) + 1;
    	if (len > max_len) 
    	{
        		lnaddr_write(addr, max_len, data & ((1 << (max_len << 3)) - 1));
        		lnaddr_write(addr + max_len, len - max_len, data >> (max_len << 3));
        		return;
    	}
	hwaddr_t hwaddr = page_translate(addr);
	hwaddr_write(hwaddr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
    uint32_t Inaddr;
	Inaddr =seg_translate(addr,len,current_sreg);
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
    uint32_t Inaddr;
	Inaddr  = seg_translate(addr,len,current_sreg);
	return lnaddr_write(addr, len, data);
}

hwaddr_t page_translate_additional(lnaddr_t addr,int* flag){
	if (cpu.cr0.protect_enable == 1 && cpu.cr0.paging == 1){
		//printf("%x\n",addr);
		uint32_t dir = addr >> 22;
		uint32_t page = (addr >> 12) & 0x3ff;
		uint32_t offset = addr & 0xfff;

		// get dir position
		uint32_t dir_start = cpu.cr3.page_directory_base;
		uint32_t dir_pos = (dir_start << 12) + (dir << 2);
		PAGE_descriptor first_content;
		first_content.page_val = hwaddr_read(dir_pos,4);
		if (first_content.p == 0) {
			*flag = 1;
			return 0;
		}

		// get page position
		uint32_t page_start = first_content.addr;
		uint32_t page_pos = (page_start << 12) + (page << 2);
		PAGE_descriptor second_content;
		second_content.page_val =  hwaddr_read(page_pos,4);
		if (second_content.p == 0){
			*flag = 2;
			return 0;
		}

		// get hwaddr
		uint32_t addr_start = second_content.addr;
		hwaddr_t hwaddr = (addr_start << 12) + offset;
		return hwaddr;
	}else return addr;
}