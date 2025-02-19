#ifndef RISCV64_INSTSELECT_H
#define RISCV64_INSTSELECT_H
#include "../../common/machine_passes/machine_selector.h"
#include "../riscv64.h"
#include <utility>
#include <vector>
class RiscV64Selector : public MachineSelector {
private:
    int cur_offset;    // 局部变量在栈中的偏移
    std::map<int, Register> reg_map;//寄存器映射
    std::map<int, int> alloca_map;//分配map
    std::map<int, Instruction> cmp_map;//比较指令
    std::map<int, bool> array_select;
    std::map<int, std::map<int,Register>> phi_merge;//每个块，需要合并的
    std::map<int, std::map<Register,Register>> phi_split;
    // 你需要保证在每个函数的指令选择结束后, cur_offset的值为局部变量所占栈空间的大小
    Register DoIOp1(ArithmeticInstruction* ins);
    Register DoIOp2(ArithmeticInstruction* ins);
    Register DoFOp1(ArithmeticInstruction* ins);
    Register DoFOp2(ArithmeticInstruction* ins);
    // TODO(): 添加更多你需要的成员变量和函数
       
public:
    RiscV64Selector(MachineUnit *dest, LLVMIR *IR) : MachineSelector(dest, IR) {}
    void SelectInstructionAndBuildCFG();
    void ClearFunctionSelectState();
    void I32toReg(Register res,int imm);
    void F32toReg(Register res,float imm);
    void PhiDestructionInCurrentFunction();
    template <class INSPTR> void ConvertAndAppend(INSPTR);
    Register GetIRReg(int ir_reg_no, MachineDataType type){
        if (reg_map.find(ir_reg_no) == reg_map.end()) {
            reg_map[ir_reg_no] = GetNewReg(type);
        }
        return reg_map[ir_reg_no];
    };
    Register GetNewReg(MachineDataType type){
        Register reg = cur_func->GetNewRegister(type.data_type, type.data_length);
        return reg;
    };
    
};
#endif