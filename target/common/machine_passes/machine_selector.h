#ifndef MACHINE_SELECTOR_H
#define MACHINE_SELECTOR_H
#include "../machine_instruction_structures/machine.h"
// 指令选择基类
class MachineSelector {
protected:
    MachineUnit *dest;
    MachineFunction *cur_func;
    MachineBlock *cur_block;
    LLVMIR *IR;//在指令选择过程中，IR 会被转换成目标机器的指令

public:
    MachineSelector(MachineUnit *dest, LLVMIR *IR) : dest(dest), IR(IR) {}
    virtual void SelectInstructionAndBuildCFG() = 0;
    MachineUnit *GetMachineUnit() { return dest; }//获取目标机器单元
};
#endif