#include "basic_register_allocation.h"

void VirtualRegisterRewrite::Execute() {
    for (auto func : unit->functions) {
        current_func = func;
        ExecuteInFunc();
    }
}

void VirtualRegisterRewrite::ExecuteInFunc() {
    auto func = current_func;
    auto alloca_func = alloc_result.find(func);
    auto block_it = func->getMachineCFG()->getSeqScanIterator();
    block_it->open();
    while (block_it->hasNext()) {
        auto block = block_it->next()->Mblock;//那个块
        for (auto it = block->begin(); it != block->end(); ++it) {
            auto ins = *it;
            //根据alloc_result将ins的虚拟寄存器重写为物理寄存器
            //复写读寄存器
            for (auto reg : ins->GetReadReg()) {
                if (reg->is_virtual) {//虚拟寄存器，需要复写
                    auto phy_reg = alloca_func->second.find(*reg)->second;
                    reg->is_virtual = false;
                    reg->reg_no = phy_reg.phy_reg_no;
                }
            }
            //复写写寄存器
            for (auto reg : ins->GetWriteReg()) {
                if (reg->is_virtual) {
                    auto phy_reg = alloca_func->second.find(*reg)->second;
                    reg->is_virtual = false;
                    reg->reg_no = phy_reg.phy_reg_no;
                }
            }
        }
    }
}

void SpillCodeGen::ExecuteInFunc(MachineFunction *function, std::map<Register, AllocResult> *alloc_result) {
    this->function = function;
    this->alloc_result = alloc_result;
    auto block_it = function->getMachineCFG()->getSeqScanIterator();
    block_it->open();
    while (block_it->hasNext()) {
        cur_block = block_it->next()->Mblock;
        for (auto it = cur_block->begin(); it != cur_block->end(); ++it) {
            auto ins = *it;
            // 根据alloc_result对ins溢出的寄存器生成溢出代码
            // 在溢出虚拟寄存器的read前插入load，在write后插入store
            // 注意load的结果寄存器是虚拟寄存器, 因为我们接下来要重新分配直到不再溢出
            for (auto reg : ins->GetReadReg()) {
                if (reg->is_virtual){//虚拟寄存器
                    auto result = alloc_result->find(*reg)->second;
                    if (result.in_mem == true) {//存在mem里
                        *reg = GenerateReadCode(it, result.stack_offset * 4, reg->type);
                    }
                }
            }
            for (auto reg : ins->GetWriteReg()) {
                if (reg->is_virtual){
                    auto result = alloc_result->find(*reg)->second;
                    if (result.in_mem == true) {
                        *reg = GenerateWriteCode(it, result.stack_offset * 4, reg->type);
                    }
                }
            }
        }
    }
}