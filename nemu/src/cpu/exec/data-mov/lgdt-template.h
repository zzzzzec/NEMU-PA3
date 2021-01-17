#include "cpu/exec/template-start.h"

#define instr lgdt

static void do_execute(){
     printf("%d \n",(int)op_src->size);
    if(op_src->size == 2)
    {
        cpu.gdtr.limit = swaddr_read(op_src->addr , 2);
        cpu.gdtr.base = swaddr_read(op_src->addr + 2, 3);
    }
    else if(op_src->size == 4)
    {
        cpu.gdtr.limit = swaddr_read(op_src->addr , 2);
        cpu.gdtr.base = swaddr_read(op_src->addr + 2, 4);
        printf("%d  %d\n",(int )cpu.gdtr.limit , (int)cpu.gdtr.base);
    }
    print_asm_template1();
}

make_instr_helper(rm)
#include "cpu/exec/template-end.h"