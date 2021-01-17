#include "cpu/exec/helper.h"

 make_helper(lgdt)
 {
     uint16_t *data;
     data = (uint16_t *)(op_src->val + data - data);
     cpu.GDTR.limit = data[0];
     cpu.GDTR.base_l = data[1];
     cpu.GDTR.base_h = data[2];

     return 5;
 } 
