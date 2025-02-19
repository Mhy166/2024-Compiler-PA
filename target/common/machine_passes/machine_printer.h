#ifndef MACHINE_PRINTER_H
#define MACHINE_PRINTER_H
#include "../machine_instruction_structures/machine.h"
// 指令打印基类
class MachinePrinter {
protected:
    MachineUnit *printee;
    MachineFunction *current_func;
    MachineBlock *cur_block;
    std::ostream &s;
    bool output_physical_reg;//用于标记是否需要输出物理寄存器

public:
    virtual void emit() = 0;
    MachinePrinter(std::ostream &s, MachineUnit *printee) : s(s), printee(printee), output_physical_reg(false) {}
    void SetOutputPhysicalReg(bool outputPhy) { output_physical_reg = outputPhy; }
    std::ostream &GetPrintStream() { return s; }
};
#endif