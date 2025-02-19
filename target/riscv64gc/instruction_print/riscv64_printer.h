#ifndef RISCV64_PRINT_H
#define RISCV64_PRINT_H
#include "../../common/machine_passes/machine_printer.h"
#include "../riscv64.h"
//为打印 RISC-V 64 指令提供了基本的框架
class RiscV64Printer : public MachinePrinter {
private:
public:
    void emit();
    void SyncFunction(MachineFunction *func);
    void SyncBlock(MachineBlock *block);
    RiscV64Printer(std::ostream &s, MachineUnit *printee) : MachinePrinter(s, printee) {}

    template <class INSPTR> void printAsm(INSPTR ins);
    template <class FIELDORPTR> void printRVfield(FIELDORPTR);
};
#endif