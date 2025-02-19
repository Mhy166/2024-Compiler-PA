#include "riscv64_lowerframe.h"

/*
    假设IR中的函数定义为f(i32 %r0, i32 %r1)
    则parameters应该存放两个虚拟寄存器%0,%1

    在LowerFrame后应当为
    add %0,a0,x0  (%0 <- a0)
    add %1,a1,x0  (%1 <- a1)

    对于浮点寄存器按照类似的方法处理即可
*/
void RiscV64LowerFrame::Execute() {
    // 在每个函数的开头处插入获取参数的指令
    for (auto func : unit->functions) {
        current_func = func;
        int offset=0;
        for (auto &b : func->blocks) {
            cur_block = b;
            if (b->getLabelId() == 0) {    // 函数入口，需要插入获取参数的指令
                int i32_cnt = 0;
                int f32_cnt = 0;
                for (auto para : func->GetParameters()) {    // 你需要在指令选择阶段正确设置parameters的值
                    if (para.type.data_type == INT64.data_type) {
                        if (i32_cnt < 8) {    // 插入使用寄存器传参的指令
                            b->push_front(rvconstructor->ConstructR(RISCV_ADD, para, GetPhysicalReg(RISCV_a0 + i32_cnt),
                                                                    GetPhysicalReg(RISCV_x0)));
                        }
                        if (i32_cnt >= 8) {    // 插入使用内存传参的指令
                            b->push_front(rvconstructor->ConstructIImm(RISCV_LD, para, GetPhysicalReg(RISCV_fp), offset));
                            offset+=8;
                        }
                        i32_cnt++;
                    } else if (para.type.data_type == FLOAT64.data_type) {    // 处理浮点数
                        if (f32_cnt < 8) {
                            b->push_front(rvconstructor->ConstructR2(RISCV_FMV_S, para, GetPhysicalReg(RISCV_fa0 + f32_cnt)));
                        }
                        if (f32_cnt >= 8) {
                            b->push_front(rvconstructor->ConstructIImm(RISCV_FLD, para, GetPhysicalReg(RISCV_fp), offset));
                            offset+=8;
                        }
                        f32_cnt++;
                    } 
                }
            }
        }
    }
    Address_Imm();
}
int save_reg[25] = {
    RISCV_s0,  RISCV_s1,  RISCV_s2,  RISCV_s3,  RISCV_s4,   RISCV_s5,   RISCV_s6,  RISCV_s7,  RISCV_s8, RISCV_s9,  RISCV_s10, RISCV_s11, 
    RISCV_fs0, RISCV_fs1,  RISCV_fs2,  RISCV_fs3, RISCV_fs4, RISCV_fs5, RISCV_fs6, RISCV_fs7, RISCV_fs8, RISCV_fs9, RISCV_fs10, RISCV_fs11, 
    RISCV_ra
};
void RiscV64LowerStack::Execute() {
    // 在函数在寄存器分配后执行
    // TODO: 在函数开头保存 函数被调者需要保存的寄存器，并开辟栈空间
    // TODO: 在函数结尾恢复 函数被调者需要保存的寄存器，并收回栈空间
    // TODO: 开辟和回收栈空间
    // 具体需要保存/恢复哪些可以查阅RISC-V函数调用约定
    for (auto func : unit->functions) {
        current_func = func;
        func->AddStackSize(25 * 8);
        for (auto &b : func->blocks) {
            cur_block = b;
            if (b->getLabelId() == 0) {
                auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                b->push_front(rvconstructor->ConstructR(RISCV_SUB, GetPhysicalReg(RISCV_sp),GetPhysicalReg(RISCV_sp), stacksz_reg));
                b->push_front(rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, func->GetStackSize()));
                b->push_front(rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_fp),GetPhysicalReg(RISCV_sp), GetPhysicalReg(RISCV_x0)));
                for(int i=0;i<25;i++){
                    if (i>=12&&i<=23) {
                        b->push_front(rvconstructor->ConstructSImm(RISCV_FSD, GetPhysicalReg(save_reg[i]),GetPhysicalReg(RISCV_sp), -(i+1)*8));
                    } else {
                        b->push_front(rvconstructor->ConstructSImm(RISCV_SD, GetPhysicalReg(save_reg[i]),GetPhysicalReg(RISCV_sp), -(i+1)*8));
                    }
                }
            }
            auto last_ins = *(b->ReverseBegin());
            auto riscv_last_ins = (RiscV64Instruction *)last_ins;
            if (riscv_last_ins->getOpcode() == RISCV_JALR) {
                if (riscv_last_ins->getRd() == GetPhysicalReg(RISCV_x0)) {
                    if (riscv_last_ins->getRs1() == GetPhysicalReg(RISCV_ra)) {
                        b->pop_back();
                        auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                        b->push_back(rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, func->GetStackSize()));
                        b->push_back(rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_sp),GetPhysicalReg(RISCV_sp), stacksz_reg));
                        for(int i=0;i<25;i++){
                            if (i>=12&&i<=23) {
                                b->push_back(rvconstructor->ConstructIImm(RISCV_FLD, GetPhysicalReg(save_reg[i]),GetPhysicalReg(RISCV_sp), -(i+1)*8));
                            } else {
                                b->push_back(rvconstructor->ConstructIImm(RISCV_LD, GetPhysicalReg(save_reg[i]),GetPhysicalReg(RISCV_sp), -(i+1)*8));
                            }
                        }
                        b->push_back(riscv_last_ins);
                    }
                }
            }
        }
    }
    // 到此我们就完成目标代码生成的所有工作了
}
void RiscV64LowerFrame::Address_Imm(){
    for(auto func:unit->functions){
        current_func=func;
        for(auto block:func->blocks){
            cur_block=block;
            for(auto it = block->begin();it!=block->end();it++){
                auto ins = (RiscV64Instruction *)(*it);
                if (ins->getImm() <= 2047 && ins->getImm() >= -2048)
                    continue;
                if (ins->getUseLabel() == true)
                    continue;
                if (ins->getOpcode() == RISCV_LW || ins->getOpcode() == RISCV_SW ||
                    ins->getOpcode() == RISCV_FLW || ins->getOpcode() == RISCV_FSW) {
                    it = block->erase(it);
                    auto imm_reg = current_func->GetNewReg(INT64);
                    auto addr_reg = current_func->GetNewReg(INT64);
                    auto imm = ins->getImm();
                    if (imm<=2047 && imm>= -2048) {
                        cur_block->insert(it,rvconstructor->ConstructIImm(RISCV_ADDIW,  imm_reg ,GetPhysicalReg(RISCV_x0), imm));
                    } else {
                        auto mid_reg = current_func->GetNewReg(INT64);                    
                        cur_block->insert(it,rvconstructor->ConstructUImm(RISCV_LUI, mid_reg, ((unsigned int)(imm + 0x800)) >> 12));
                        cur_block->insert(it,rvconstructor->ConstructIImm(RISCV_ADDIW,  imm_reg  ,mid_reg, (imm << 20) >> 20));
                    }
                    Register addrbase_reg;
                    if (ins->getOpcode() == RISCV_LW ||ins->getOpcode() == RISCV_FLW) {
                        addrbase_reg = ins->getRs1();
                        ins->setRs1(addr_reg);
                    } else {
                        addrbase_reg = ins->getRs2();
                        ins->setRs2(addr_reg);
                    }
                    cur_block->insert(it, rvconstructor->ConstructR(RISCV_ADD, addr_reg, addrbase_reg, imm_reg));
                    ins->setImm(0);
                    cur_block->insert(it,ins);
                    --it;
                } else if (OpTable[ins->getOpcode()].ins_formattype == RvOpInfo::I_type) {
                    it = block->erase(it);
                    auto imm_reg = current_func->GetNewReg(INT64);
                    auto imm = ins->getImm();
                    if (imm<=2047 && imm>= -2048) {
                        cur_block->insert(it,rvconstructor->ConstructIImm(RISCV_ADDIW,  imm_reg ,GetPhysicalReg(RISCV_x0), imm));
                    } else {
                        auto mid_reg = current_func->GetNewReg(INT64);                    
                        cur_block->insert(it,rvconstructor->ConstructUImm(RISCV_LUI, mid_reg, ((unsigned int)(imm + 0x800)) >> 12));
                        cur_block->insert(it,rvconstructor->ConstructIImm(RISCV_ADDIW,  imm_reg  ,mid_reg, (imm << 20) >> 20));
                    }
                    ins->setRs2(imm_reg);
                    ins->setImm(0);
                    switch (ins->getOpcode()) {
                        case RISCV_SLLI:
                            ins->setOpcode(RISCV_SLL, false);
                            break;
                        case RISCV_SRLI:
                            ins->setOpcode(RISCV_SRL, false);
                            break;
                        case RISCV_SRAI:
                            ins->setOpcode(RISCV_SRA, false);
                            break;
                        case RISCV_ADDI:
                            ins->setOpcode(RISCV_ADD, false);
                            break;
                        case RISCV_XORI:
                            ins->setOpcode(RISCV_XOR, false);
                            break;
                        case RISCV_ORI:
                            ins->setOpcode(RISCV_OR, false);
                            break;
                        case RISCV_ANDI:
                            ins->setOpcode(RISCV_AND, false);
                            break;
                        case RISCV_SLTI:
                            ins->setOpcode(RISCV_SLT, false);
                            break;
                        case RISCV_SLTIU:
                            ins->setOpcode(RISCV_SLTU, false);
                            break;
                        case RISCV_SLLIW:
                            ins->setOpcode(RISCV_SLLW, false);
                            break;
                        case RISCV_SRLIW:
                            ins->setOpcode(RISCV_SRLW, false);
                            break;
                        case RISCV_SRAIW:
                            ins->setOpcode(RISCV_SRAW, false);
                            break;
                        case RISCV_ADDIW:
                            ins->setOpcode(RISCV_ADDW, false);
                            break;
                    }
                    block->insert(it, ins);
                    --it;
                }
            }
        }
    }
}