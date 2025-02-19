#include "riscv64_instSelect.h"
#include <sstream>
#include <utility>
void RiscV64Selector::I32toReg(Register res,int imm){
    if (imm<=2047 && imm>= -2048) {//第12位不是1，数小，直接加！
        cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDIW, res,GetPhysicalReg(RISCV_x0), imm));
    }  else {
        auto mid_reg = GetNewReg(INT64);                    
        cur_block->push_back(rvconstructor->ConstructUImm(RISCV_LUI, mid_reg, ((unsigned int)(imm + 0x800)) >> 12));
        cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDIW, res ,mid_reg, (imm << 20) >> 20));
    }
}
void RiscV64Selector::F32toReg(Register res,float imm){
    auto mid_reg = cur_func->GetNewReg(INT64);
    I32toReg(mid_reg, *(int*)&imm);
    cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, res, mid_reg));
}
Register RiscV64Selector::DoIOp1(ArithmeticInstruction* ins){
    Register op;
    if(ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32){
        op=GetNewReg(INT64);
        auto imm = (ImmI32Operand*)ins->GetOperand1();
        I32toReg(op,imm->GetIntImmVal());
    }else{
        auto reg = (RegOperand*)ins->GetOperand1();
        op=GetIRReg(reg->GetRegNo(), INT64);
    }
    return op;
}
Register RiscV64Selector::DoIOp2(ArithmeticInstruction* ins){
    Register op;
    if(ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32){
        op=GetNewReg(INT64);
        auto imm = (ImmI32Operand*)ins->GetOperand2();
        I32toReg(op,imm->GetIntImmVal());
    }else{
        auto reg = (RegOperand*)ins->GetOperand2();
        op=GetIRReg(reg->GetRegNo(), INT64);
    }
    return op;
}
Register RiscV64Selector::DoFOp1(ArithmeticInstruction* ins){
    Register op;
    if(ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32){
        op=GetNewReg(FLOAT64);
        auto imm = (ImmF32Operand*)ins->GetOperand1();
        F32toReg(op,imm->GetFloatVal());
    }else{
        auto reg = (RegOperand*)ins->GetOperand1();
        op=GetIRReg(reg->GetRegNo(), FLOAT64);
    }
    return op;
}
Register RiscV64Selector::DoFOp2(ArithmeticInstruction* ins){
    Register op;
    if(ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32){
        op=GetNewReg(FLOAT64);
        auto imm = (ImmF32Operand*)ins->GetOperand2();
        F32toReg(op,imm->GetFloatVal());
    }else{
        auto reg = (RegOperand*)ins->GetOperand2();
        op=GetIRReg(reg->GetRegNo(), FLOAT64);
    }
    return op;
}
extern RiscV64InstructionConstructor *rvconstructor;
// enum LLVMType type;op1;op2;result;
template <> void RiscV64Selector::ConvertAndAppend<ArithmeticInstruction *>(ArithmeticInstruction *ins) {
    //算数指令,type有：ADD FADD SUB FSUB MUL FMUL DIV FDIV MOD
    if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::ADD) {//整数
        Register op1=DoIOp1(ins);
        Register op2=DoIOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), INT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_ADDW, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::SUB){//减法
        Register op1=DoIOp1(ins);
        Register op2=DoIOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), INT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_SUBW, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::MUL){
        Register op1=DoIOp1(ins);
        Register op2=DoIOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), INT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_MULW, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::DIV){
        Register op1=DoIOp1(ins);
        Register op2=DoIOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), INT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_DIVW, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::MOD){
        Register op1=DoIOp1(ins);
        Register op2=DoIOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), INT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_REMW, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::FADD){
        Register op1=DoFOp1(ins);
        Register op2=DoFOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), FLOAT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_FADD_S, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::FSUB){
        Register op1=DoFOp1(ins);
        Register op2=DoFOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), FLOAT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_FSUB_S, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::FMUL){
        Register op1=DoFOp1(ins);
        Register op2=DoFOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), FLOAT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_FMUL_S, res, op1, op2));
    }else if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::FDIV){
        Register op1=DoFOp1(ins);
        Register op2=DoFOp2(ins);
        Register res = GetIRReg(ins->GetResultRegNo(), FLOAT64);
        cur_block->push_back(rvconstructor->ConstructR(RISCV_FDIV_S, res, op1, op2));
    }
}
//√
    template <> void RiscV64Selector::ConvertAndAppend<IcmpInstruction *>(IcmpInstruction *ins) {
        auto res = (RegOperand *)ins->GetResult();
        auto res_reg = GetIRReg(res->GetRegNo(), INT64);
        cmp_map[res_reg.reg_no] = ins;
    }
    template <> void RiscV64Selector::ConvertAndAppend<FcmpInstruction *>(FcmpInstruction *ins) {
        auto res = (RegOperand *)ins->GetResult();
        auto res_reg = GetIRReg(res->GetRegNo(), INT64);
        cmp_map[res_reg.reg_no] = ins;
    }
    //在栈上分配
    template <> void RiscV64Selector::ConvertAndAppend<AllocaInstruction *>(AllocaInstruction *ins) {
        auto ptr = (RegOperand *)ins->GetResult();
        alloca_map[ptr->GetRegNo()] = cur_offset;
        int alloca_size = 1;
        for (auto d : ins->GetDims()) {
            alloca_size *= d;
        }
        cur_offset += (alloca_size*4);
    }
    template <> void RiscV64Selector::ConvertAndAppend<BrUncondInstruction *>(BrUncondInstruction *ins) {
        //对于无条件跳转，生成一条无条件指令
        auto br_ins = (BrUncondInstruction*)ins;
        auto ir_label = (LabelOperand*)br_ins->GetDestLabel();
        RiscVLabel j_label=RiscVLabel(ir_label->GetLabelNo());
        cur_block->push_back(rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0),j_label));
    }
    //完成Load--pointer,result(reg)
    template <> void RiscV64Selector::ConvertAndAppend<LoadInstruction *>(LoadInstruction *ins) {
        //load要区分全局变量还是局部
        if(ins->GetPointer()->GetOperandType()==BasicOperand::GLOBAL){//全局
            //分两次加载高低位
            auto global_ptr = (GlobalOperand *)ins->GetPointer();
            auto res = (RegOperand *)ins->GetResult();

            Register reg_hi = GetNewReg(INT64);
            if (ins->GetDataType() == BasicInstruction::LLVMType::I32||ins->GetDataType() == BasicInstruction::LLVMType::PTR) {
                Register rd = GetIRReg(res->GetRegNo(), INT64);//需要和IR的reg有关联
                //生成lui %reg_hi, %hi(x)
                auto lui_ins = rvconstructor->ConstructULabel(RISCV_LUI, reg_hi, RiscVLabel(global_ptr->GetName(), true));
                //生成lw  %rd, %lo(x)(%reg_hi)
                auto lw_ins = rvconstructor->ConstructILabel(RISCV_LW, rd, reg_hi, RiscVLabel(global_ptr->GetName(), false));
                cur_block->push_back(lui_ins);
                cur_block->push_back(lw_ins);
            } else if (ins->GetDataType() == BasicInstruction::LLVMType::FLOAT32) {
                Register rd = GetIRReg(res->GetRegNo(), FLOAT64);
                auto lui_ins = rvconstructor->ConstructULabel(RISCV_LUI, reg_hi, RiscVLabel(global_ptr->GetName(), true));
                auto lw_ins = rvconstructor->ConstructILabel(RISCV_FLW, rd, reg_hi, RiscVLabel(global_ptr->GetName(), false));
                cur_block->push_back(lui_ins);
                cur_block->push_back(lw_ins);
            }
        }else{//局部，REG类型,这里如果是数组也没关系，一条lw，计算好偏移
            auto reg_ptr = (RegOperand *)ins->GetPointer();
            auto res = (RegOperand *)ins->GetResult();
            if (ins->GetDataType() == BasicInstruction::LLVMType::I32||ins->GetDataType() == BasicInstruction::LLVMType::PTR) {
                //结果
                Register rd = GetIRReg(res->GetRegNo(), INT64);
                //lw  %rd, 0(%rs)
                if (alloca_map.find(reg_ptr->GetRegNo()) == alloca_map.end()) {//分配表里没有
                    Register ptr = GetIRReg(reg_ptr->GetRegNo(), INT64);    
                    auto lw_ins = rvconstructor->ConstructIImm(RISCV_LW, rd, ptr, 0);
                    cur_block->push_back(lw_ins);
                } else {//分配表里有,计算偏移,由sp栈指针寻址
                    auto lw_ins = rvconstructor->ConstructIImm(RISCV_LW, rd, GetPhysicalReg(RISCV_sp), alloca_map[reg_ptr->GetRegNo()]);
                    ((RiscV64Function *)cur_func)->use_stack_ins.push_back(lw_ins);
                    cur_block->push_back(lw_ins);
                }
            } else if (ins->GetDataType() == BasicInstruction::LLVMType::FLOAT32) {
                Register rd = GetIRReg(res->GetRegNo(), FLOAT64);
                if (alloca_map.find(reg_ptr->GetRegNo()) == alloca_map.end()) {
                    Register ptr = GetIRReg(reg_ptr->GetRegNo(), INT64);
                    auto lw_ins = rvconstructor->ConstructIImm(RISCV_FLW, rd, ptr, 0);
                    cur_block->push_back(lw_ins);
                } else {
                    auto lw_ins = rvconstructor->ConstructIImm(RISCV_FLW, rd, GetPhysicalReg(RISCV_sp), alloca_map[reg_ptr->GetRegNo()]);
                    ((RiscV64Function *)cur_func)->use_stack_ins.push_back(lw_ins);
                    cur_block->push_back(lw_ins);
                }
            }
        }
    }
    //完成Store--pointer,value(reg/i/f)
    template <> void RiscV64Selector::ConvertAndAppend<StoreInstruction *>(StoreInstruction *ins) {
        Register value_reg;
        if (ins->GetValue()->GetOperandType() == BasicOperand::REG) {//寄存器的话，先前应该是存过！直接找
            auto val_reg = (RegOperand *)ins->GetValue();
            if (ins->GetDataType() == BasicInstruction::LLVMType::I32) {
                value_reg = GetIRReg(val_reg->GetRegNo(), INT64);
            } else if (ins->GetDataType() == BasicInstruction::LLVMType::FLOAT32) {
                value_reg = GetIRReg(val_reg->GetRegNo(), FLOAT64);
            }
        }else if (ins->GetValue()->GetOperandType() == BasicOperand::IMMI32) {//把I32的值存入
            auto val = (ImmI32Operand *)ins->GetValue();
            value_reg = GetNewReg(INT64);
            I32toReg(value_reg, val->GetIntImmVal());
        }else if (ins->GetValue()->GetOperandType() == BasicOperand::IMMF32) {
            auto val = (ImmF32Operand *)ins->GetValue();
            value_reg = GetNewReg(FLOAT64);
            F32toReg(value_reg, val->GetFloatVal());
        }
        //值寄存器安排好了，接下来存入全局变量或者局部变量
        if (ins->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {//全局
            auto global = (GlobalOperand *)ins->GetPointer();
            auto reg_hi = GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructULabel(RISCV_LUI, reg_hi, RiscVLabel(global->GetName(), true)));
            if (ins->GetDataType() == BasicInstruction::LLVMType::I32) {
                cur_block->push_back(rvconstructor->ConstructSLabel(RISCV_SW, value_reg, reg_hi, RiscVLabel(global->GetName(), false)));
            } else if (ins->GetDataType() == BasicInstruction::LLVMType::FLOAT32) {
                cur_block->push_back(rvconstructor->ConstructSLabel(RISCV_FSW, value_reg, reg_hi, RiscVLabel(global->GetName(), false)));
            }
        }else if (ins->GetPointer()->GetOperandType() == BasicOperand::REG) {//局部
            auto reg_ptr = (RegOperand *)ins->GetPointer();
            if (ins->GetDataType() == BasicInstruction::LLVMType::I32) {
                if (alloca_map.find(reg_ptr->GetRegNo()) == alloca_map.end()) {//分配表里没有
                    auto ptr_reg = GetIRReg(reg_ptr->GetRegNo(), INT64);
                    cur_block->push_back(rvconstructor->ConstructSImm(RISCV_SW, value_reg, ptr_reg, 0));
                } else {
                    auto sw_ins = rvconstructor->ConstructSImm(RISCV_SW, value_reg, GetPhysicalReg(RISCV_sp), alloca_map[reg_ptr->GetRegNo()]);
                    ((RiscV64Function *)cur_func)->use_stack_ins.push_back(sw_ins);
                    cur_block->push_back(sw_ins);
                }
            } else if (ins->GetDataType() == BasicInstruction::LLVMType::FLOAT32) {
                if (alloca_map.find(reg_ptr->GetRegNo()) == alloca_map.end()) {
                    auto ptr_reg = GetIRReg(reg_ptr->GetRegNo(), INT64);
                    cur_block->push_back(rvconstructor->ConstructSImm(RISCV_FSW, value_reg, ptr_reg, 0));
                } else {
                    auto sw_ins = rvconstructor->ConstructSImm(RISCV_FSW, value_reg, GetPhysicalReg(RISCV_sp), alloca_map[reg_ptr->GetRegNo()]);
                    ((RiscV64Function *)cur_func)->use_stack_ins.push_back(sw_ins);
                    cur_block->push_back(sw_ins);
                }
            }
        } 
    }
    //完成Ret--ret_val
    template <> void RiscV64Selector::ConvertAndAppend<RetInstruction *>(RetInstruction *ins) {
        //存好对应返回值
        if (ins->GetRetVal() != NULL) {
            if (ins->GetRetVal()->GetOperandType() == BasicOperand::IMMI32) {//返回值是立即整数，需要存入a0
                auto imm = ((ImmI32Operand *)ins->GetRetVal())->GetIntImmVal();
                I32toReg(GetPhysicalReg(RISCV_a0),imm);
            } else if (ins->GetRetVal()->GetOperandType() == BasicOperand::IMMF32) {
                auto imm = ((ImmF32Operand *)ins->GetRetVal())->GetFloatVal();
                F32toReg(GetPhysicalReg(RISCV_fa0),imm);
            } else if (ins->GetRetVal()->GetOperandType() == BasicOperand::REG) {
                if (ins->GetType() == BasicInstruction::LLVMType::I32) {
                    auto ret_reg = (RegOperand *)ins->GetRetVal();
                    auto reg = GetIRReg(ret_reg->GetRegNo(),INT64);//获取虚拟寄存器！
                    cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a0), reg, GetPhysicalReg(RISCV_x0)));
                } else if (ins->GetType() == BasicInstruction::LLVMType::FLOAT32) {
                    auto ret_reg = (RegOperand *)ins->GetRetVal();
                    auto reg = GetIRReg(ret_reg->GetRegNo(),FLOAT64);//获取虚拟寄存器！
                    cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_S, GetPhysicalReg(RISCV_fa0), reg));
                }
            }
        }
        //返回指令
        auto ret_ins = rvconstructor->ConstructIImm(RISCV_JALR, GetPhysicalReg(RISCV_x0), GetPhysicalReg(RISCV_ra), 0);
        if (ins->GetType() == BasicInstruction::I32) {
            ret_ins->setRetType(1);//读取a0
        } else if (ins->GetType() == BasicInstruction::FLOAT32) {
            ret_ins->setRetType(2);//读取fa0
        } else {
            ret_ins->setRetType(0);
        }
        cur_block->push_back(ret_ins);
    }
//Br--cond（result）,trueLabel,falseLabel cond可以找到icmp op1，op2，type，cond
template <> void RiscV64Selector::ConvertAndAppend<BrCondInstruction *>(BrCondInstruction *ins) {
    auto cond = (RegOperand *)ins->GetCond();
    auto cond_reg = GetIRReg(cond->GetRegNo(), INT64);
    auto cmp_ins = cmp_map[cond_reg.reg_no];//找到那条指令！
    if(cmp_ins->GetOpcode()==BasicInstruction::LLVMIROpcode::ICMP){
        Register op1;
        auto icmp_ins=(IcmpInstruction*)cmp_ins;
        if(icmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMI32){
            op1=GetNewReg(INT64);
            auto imm = (ImmI32Operand*)icmp_ins->GetOp1();
            I32toReg(op1,imm->GetIntImmVal());
        }else{
            auto reg = (RegOperand*)icmp_ins->GetOp1();
            op1=GetIRReg(reg->GetRegNo(), INT64);
        }
        Register op2;
        if(icmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMI32){
            op2=GetNewReg(INT64);
            auto imm = (ImmI32Operand*)icmp_ins->GetOp2();
            I32toReg(op2,imm->GetIntImmVal());
        }else{
            auto reg = (RegOperand*)icmp_ins->GetOp2();
            op2=GetIRReg(reg->GetRegNo(), INT64);
        }
        int opcode;
        if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::eq){
            opcode=RISCV_BEQ;
        }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::ne){
            opcode=RISCV_BNE;
        }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::sge){
            opcode=RISCV_BGE;
        }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::sgt){
            opcode=RISCV_BGT;
        }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::sle){
            opcode=RISCV_BLE;
        }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::slt){
            opcode=RISCV_BLT;
        }
        auto true_label = (LabelOperand *)ins->GetTrueLabel();
        auto false_label = (LabelOperand *)ins->GetFalseLabel();
        cur_block->push_back(rvconstructor->ConstructBLabel(opcode, op1, op2,RiscVLabel(true_label->GetLabelNo())));
        cur_block->push_back(rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo())));
    }else{//FCMP
        Register op1;
        auto fcmp_ins=(FcmpInstruction*)cmp_ins;
        if(fcmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMF32){
            op1=GetNewReg(FLOAT64);
            auto imm = (ImmF32Operand*)fcmp_ins->GetOp1();
            F32toReg(op1,imm->GetFloatVal());
        }else{
            auto reg = (RegOperand*)fcmp_ins->GetOp1();
            op1=GetIRReg(reg->GetRegNo(), FLOAT64);
        }
        Register op2;
        if(fcmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMF32){
            op2=GetNewReg(FLOAT64);
            auto imm = (ImmF32Operand*)fcmp_ins->GetOp2();
            F32toReg(op2,imm->GetFloatVal());
        }else{
            auto reg = (RegOperand*)fcmp_ins->GetOp2();
            op2=GetIRReg(reg->GetRegNo(), FLOAT64);
        }
        auto cmp_rd=GetNewReg(INT64);
        int opcode;
        if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OEQ){
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S,cmp_rd, op1, op2));
            opcode=RISCV_BNE;
        }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::ONE){
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S, cmp_rd, op1, op2));
            opcode = RISCV_BEQ;
        }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OGE){
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLE_S, cmp_rd, op2, op1));
            opcode = RISCV_BNE;
        }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OGT){
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, cmp_rd, op2, op1));
            opcode = RISCV_BNE;
        }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OLE){
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLE_S, cmp_rd, op1, op2));
            opcode = RISCV_BNE;
        }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OLT){
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, cmp_rd, op1, op2));
            opcode = RISCV_BNE;
        }
        auto true_label = (LabelOperand *)ins->GetTrueLabel();
        auto false_label = (LabelOperand *)ins->GetFalseLabel();
        cur_block->push_back(rvconstructor->ConstructBLabel(opcode, cmp_rd, GetPhysicalReg(RISCV_x0),RiscVLabel(true_label->GetLabelNo())));
        cur_block->push_back(rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo())));
    }
}
//Zext result value
    template <> void RiscV64Selector::ConvertAndAppend<ZextInstruction *>(ZextInstruction *ins) {
        auto src_reg = GetIRReg(((RegOperand *)ins->GetSrc())->GetRegNo(), INT64);
        auto res_reg = GetIRReg(((RegOperand *)ins->GetResultReg())->GetRegNo(), INT64);
        auto cmp_ins = cmp_map[src_reg.reg_no];//找到那条指令！
        if(cmp_ins->GetOpcode()==BasicInstruction::LLVMIROpcode::ICMP){
            Register op1;
            auto icmp_ins=(IcmpInstruction*)cmp_ins;
            if(icmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMI32){
                op1=GetNewReg(INT64);
                auto imm = (ImmI32Operand*)icmp_ins->GetOp1();
                I32toReg(op1,imm->GetIntImmVal());
            }else{
                auto reg = (RegOperand*)icmp_ins->GetOp1();
                op1=GetIRReg(reg->GetRegNo(), INT64);
            }
            Register op2;
            if(icmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMI32){
                op2=GetNewReg(INT64);
                auto imm = (ImmI32Operand*)icmp_ins->GetOp2();
                I32toReg(op2,imm->GetIntImmVal());
            }else{
                auto reg = (RegOperand*)icmp_ins->GetOp2();
                op2=GetIRReg(reg->GetRegNo(), INT64);
            }
            Register mid_reg = GetNewReg(INT64);
            if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::eq){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SUBW, mid_reg, op1, op2));
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLTIU, res_reg, mid_reg, 1));
                return;
            }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::ne){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SUBW, mid_reg, op1, op2));
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLTU, res_reg, GetPhysicalReg(RISCV_x0), mid_reg));
                return;
            }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::sge){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, mid_reg, op1, op2));
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_XORI, res_reg, mid_reg, 1));
                return;
            }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::sgt){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, res_reg, op2, op1));
                return;
            }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::sle){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, mid_reg, op2, op1));
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_XORI, res_reg, mid_reg, 1));
                return;
            }else if(icmp_ins->GetCond()==BasicInstruction::IcmpCond::slt){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, res_reg, op1, op2));
                return;
            }
        }else{//FCMP
            Register op1;
            auto fcmp_ins=(FcmpInstruction*)cmp_ins;
            if(fcmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMF32){
                op1=GetNewReg(FLOAT64);
                auto imm = (ImmF32Operand*)fcmp_ins->GetOp1();
                F32toReg(op1,imm->GetFloatVal());
            }else{
                auto reg = (RegOperand*)fcmp_ins->GetOp1();
                op1=GetIRReg(reg->GetRegNo(), FLOAT64);
            }
            Register op2;
            if(fcmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMF32){
                op2=GetNewReg(FLOAT64);
                auto imm = (ImmF32Operand*)fcmp_ins->GetOp2();
                F32toReg(op2,imm->GetFloatVal());
            }else{
                auto reg = (RegOperand*)fcmp_ins->GetOp2();
                op2=GetIRReg(reg->GetRegNo(), FLOAT64);
            }
            if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OEQ){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S, res_reg, op1, op2));
            }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::ONE){
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S, res_reg, op1, op2));
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_XORI, res_reg, res_reg, 1));
            }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OGE){//>=
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FLE_S, res_reg, op2, op1));
            }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OGT){//>
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, res_reg, op2, op1));
            }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OLE){//<=
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FLE_S, res_reg, op1, op2));
            }else if(fcmp_ins->GetCond()==BasicInstruction::FcmpCond::OLT){//<
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, res_reg, op1, op2));
            }
        }
    }
template <> void RiscV64Selector::ConvertAndAppend<CallInstruction *>(CallInstruction *ins) {
    int ireg_num = 0;
    int freg_num = 0;
    int stack_para_num = 0;
    if (ins->GetFunctionName() == std::string("llvm.memset.p0.i32")) {//特殊处理
        {
            int ptrreg_no = ((RegOperand *)ins->GetParameterList()[0].second)->GetRegNo();
            if (alloca_map.find(ptrreg_no) == alloca_map.end()) {
                cur_block->push_back(rvconstructor->ConstructR(RISCV_ADDW,GetPhysicalReg(RISCV_a0), GetIRReg(ptrreg_no, INT64), GetPhysicalReg(RISCV_x0)));
            } else {
                auto sp_offset = alloca_map[ptrreg_no];
                auto rv_ins = rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->use_stack_ins.push_back(rv_ins);
                cur_block->push_back(rv_ins);
            }
        }
        {
            auto imm_op = (ImmI32Operand *)(ins->GetParameterList()[1].second);
            I32toReg(GetPhysicalReg(RISCV_a1), imm_op->GetIntImmVal());
        }
        {
            if (ins->GetParameterList()[2].second->GetOperandType() == BasicOperand::IMMI32) {
                int arr_sz = ((ImmI32Operand *)ins->GetParameterList()[2].second)->GetIntImmVal();
                I32toReg(GetPhysicalReg(RISCV_a2), arr_sz);
            } else {
                int sizereg_no = ((RegOperand *)ins->GetParameterList()[2].second)->GetRegNo();
                if (alloca_map.find(sizereg_no) == alloca_map.end()) {
                    cur_block->push_back(rvconstructor->ConstructR(RISCV_ADDW,GetPhysicalReg(RISCV_a2), GetIRReg(sizereg_no, INT64), GetPhysicalReg(RISCV_x0)));
                } else {
                    auto sp_offset = alloca_map[sizereg_no];
                    auto rv_ins = rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_a2),GetPhysicalReg(RISCV_sp), sp_offset);
                    ((RiscV64Function *)cur_func)->use_stack_ins.push_back(rv_ins);
                    cur_block->push_back(rv_ins);
                }
            }
        }
        // call
        cur_block->push_back(rvconstructor->ConstructCall(RISCV_CALL, "memset", 3, 0));
        return;
    }else{//result，args
        for (auto [type, arg_op] : ins->GetParameterList()) {
            if (type == BasicInstruction::LLVMType::I32||type == BasicInstruction::LLVMType::PTR) {//I32参数
                if (ireg_num < 8) {//还可用寄存器
                    if (arg_op->GetOperandType() == BasicOperand::REG) {
                        auto arg_reg_op = (RegOperand *)arg_op;
                        auto arg_reg = GetIRReg(arg_reg_op->GetRegNo(), INT64);
                        if (alloca_map.find(arg_reg_op->GetRegNo()) == alloca_map.end()) {
                            cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD, 
                                GetPhysicalReg(RISCV_a0 + ireg_num),arg_reg,GetPhysicalReg(RISCV_x0)));
                        } else {
                            auto sp_offset = alloca_map[arg_reg_op->GetRegNo()];
                            cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDI, 
                                GetPhysicalReg(RISCV_a0 + ireg_num), GetPhysicalReg(RISCV_sp), sp_offset));
                        }
                    } else if (arg_op->GetOperandType() == BasicOperand::IMMI32) {
                        auto arg_imm_op = (ImmI32Operand *)arg_op;
                        auto arg_imm = arg_imm_op->GetIntImmVal();
                        I32toReg(GetPhysicalReg(RISCV_a0 + ireg_num), arg_imm);
                    } else if (arg_op->GetOperandType() == BasicOperand::GLOBAL) {
                        auto mid_reg = GetNewReg(INT64);
                        auto arg_global = (GlobalOperand *)arg_op;
                        cur_block->push_back(rvconstructor->ConstructULabel(RISCV_LUI, 
                                mid_reg, RiscVLabel(arg_global->GetName(), true)));
                        cur_block->push_back(rvconstructor->ConstructILabel(RISCV_ADDI, 
                                GetPhysicalReg(RISCV_a0 + ireg_num),mid_reg,RiscVLabel(arg_global->GetName(), false)));
                    }
                }
                ireg_num++;
            } else if (type == BasicInstruction::LLVMType::FLOAT32) {
                if (freg_num < 8) {//还可用寄存器
                    if (arg_op->GetOperandType() == BasicOperand::REG) {
                        auto arg_reg_op = (RegOperand *)arg_op;
                        auto arg_reg = GetIRReg(arg_reg_op->GetRegNo(), FLOAT64);
                        cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_S, 
                                GetPhysicalReg(RISCV_fa0 + freg_num),arg_reg));
                    }else if (arg_op->GetOperandType() == BasicOperand::IMMF32) {
                        auto arg_imm_op = (ImmF32Operand *)arg_op;
                        auto arg_imm = arg_imm_op->GetFloatVal();
                        F32toReg(GetPhysicalReg(RISCV_fa0 + freg_num), arg_imm);
                    }
                }
                freg_num++;
            }
        }
        stack_para_num += (ireg_num - 8 > 0?(ireg_num - 8):0);
        stack_para_num += (freg_num - 8 > 0?(freg_num - 8):0);
        //处理带栈上参数的
        if (stack_para_num != 0) {
            ireg_num = 0; 
            freg_num = 0;
            int arg_off = 0;
            for (auto [type, arg_op] : ins->GetParameterList()) {
                if (type == BasicInstruction::LLVMType::I32||type == BasicInstruction::LLVMType::PTR) {
                    if (ireg_num < 8) {
                    } else {
                        if (arg_op->GetOperandType() == BasicOperand::REG) {
                            auto arg_reg_op = (RegOperand *)arg_op;
                            auto arg_reg = GetIRReg(arg_reg_op->GetRegNo(), INT64);
                            if (alloca_map.find(arg_reg_op->GetRegNo()) == alloca_map.end()) {
                                cur_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
                            } else {
                                auto sp_offset = alloca_map[arg_reg_op->GetRegNo()];
                                auto mid_reg = GetNewReg(INT64);
                                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDI, mid_reg, GetPhysicalReg(RISCV_sp), sp_offset));
                                cur_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, mid_reg, GetPhysicalReg(RISCV_sp), arg_off));
                            }
                        } else if (arg_op->GetOperandType() == BasicOperand::IMMI32) {
                            auto arg_imm_op = (ImmI32Operand *)arg_op;
                            auto arg_imm = arg_imm_op->GetIntImmVal();
                            auto imm_reg = GetNewReg(INT64);
                            I32toReg(imm_reg, arg_imm);
                            cur_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, imm_reg, GetPhysicalReg(RISCV_sp), arg_off));
                        } else if (arg_op->GetOperandType() == BasicOperand::GLOBAL) {
                            auto glb_reg1 = GetNewReg(INT64);
                            auto glb_reg2 = GetNewReg(INT64);
                            auto arg_glbop = (GlobalOperand *)arg_op;
                            cur_block->push_back(rvconstructor->ConstructULabel(RISCV_LUI, glb_reg1, RiscVLabel(arg_glbop->GetName(), true)));
                            cur_block->push_back(rvconstructor->ConstructILabel(RISCV_ADDI, glb_reg2, glb_reg1,RiscVLabel(arg_glbop->GetName(), false)));
                            cur_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, glb_reg2, GetPhysicalReg(RISCV_sp), arg_off));
                        }
                        arg_off += 8;
                    }
                    ireg_num++;
                } else if (type == BasicInstruction::LLVMType::FLOAT32) {  
                    if(freg_num < 8){
                    } else{
                        if (arg_op->GetOperandType() == BasicOperand::REG) {
                            auto arg_regop = (RegOperand *)arg_op;
                            auto arg_reg = GetIRReg(arg_regop->GetRegNo(), FLOAT64);
                            cur_block->push_back(rvconstructor->ConstructSImm(RISCV_FSD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
                        } else if (arg_op->GetOperandType() == BasicOperand::IMMF32) {
                            auto arg_imm_op = (ImmF32Operand *)arg_op;
                            auto arg_imm = arg_imm_op->GetFloatVal();
                            auto imm_reg = GetNewReg(INT64);
                            I32toReg(imm_reg,*(int *)&arg_imm );
                            cur_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, imm_reg, GetPhysicalReg(RISCV_sp), arg_off));
                        } else {
                            ERROR("Unexpected Operand type");
                        }
                        arg_off += 8;
                    }
                    freg_num++;
                } 
            }
        }

        // Call Label
        auto call_funcname = ins->GetFunctionName();
        if (ireg_num > 8) {
            ireg_num = 8;
        }
        if (freg_num > 8) {
            freg_num = 8;
        }
        // Return Value
        auto return_type = ins->GetRetType();
        auto result_op = (RegOperand *)ins->GetResultReg();
        cur_block->push_back(rvconstructor->ConstructCall(RISCV_CALL, call_funcname, ireg_num, freg_num));
        cur_func->UpdateParaSize(stack_para_num * 8);
        if (return_type == BasicInstruction::LLVMType::I32) {
            auto res_reg = GetIRReg(result_op->GetRegNo(), INT64);
            cur_block->push_back(rvconstructor->ConstructR(RISCV_ADDW, res_reg, GetPhysicalReg(RISCV_a0),GetPhysicalReg(RISCV_x0)));
        } else if (return_type == BasicInstruction::LLVMType::FLOAT32) {
            auto res_reg = GetIRReg(result_op->GetRegNo(), FLOAT64);
            cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_S, res_reg, GetPhysicalReg(RISCV_fa0)));
        } 
    }
}
//res(i) value(f)
template <> void RiscV64Selector::ConvertAndAppend<FptosiInstruction *>(FptosiInstruction *ins) {
    auto src = ins->GetSrc();
    auto res = (RegOperand *)ins->GetResultReg();
    if (src->GetOperandType() == BasicOperand::REG) {
        auto reg_src = (RegOperand *)src;
        cur_block->push_back(rvconstructor->ConstructR2(RISCV_FCVT_W_S, GetIRReg(res->GetRegNo(), INT64),
                            GetIRReg(reg_src->GetRegNo(), FLOAT64)));
    } else if (src->GetOperandType() == BasicOperand::IMMF32) {
        auto f_src = (ImmF32Operand *)src;
        I32toReg(GetIRReg(res->GetRegNo(), INT64), f_src->GetFloatVal());
    }
}
//res(f) value(i)
template <> void RiscV64Selector::ConvertAndAppend<SitofpInstruction *>(SitofpInstruction *ins) {
    auto src = ins->GetSrc();
    auto res = (RegOperand *)ins->GetResultReg();
    if (src->GetOperandType() == BasicOperand::REG) {
        auto reg_src = (RegOperand *)src;
        cur_block->push_back(rvconstructor->ConstructR2(RISCV_FCVT_S_W, GetIRReg(res->GetRegNo(), FLOAT64),
                            GetIRReg(reg_src->GetRegNo(), INT64)));
    } else if (src->GetOperandType() == BasicOperand::IMMI32) {
        auto i_src = (ImmI32Operand *)src;
        auto mid_reg = GetNewReg(INT64);
        I32toReg(mid_reg, i_src->GetIntImmVal());
        cur_block->push_back(rvconstructor->ConstructR2(RISCV_FCVT_S_W, GetIRReg(res->GetRegNo(), FLOAT64), mid_reg));
    }
}
template <> void RiscV64Selector::ConvertAndAppend<GetElementptrInstruction *>(GetElementptrInstruction *ins) {
    bool flag = false;
    auto ptr = ins->GetPtrVal();
    if(ptr->GetOperandType()==BasicOperand::REG){
        auto ptr_reg = (RegOperand*)ptr;
        if(array_select.find(ptr_reg->GetRegNo())!=array_select.end()){
            flag = true; //形参
        }
    }
    int size = 1;
    for (auto dim : ins->GetDims()) {
        size *= dim;
    }
    auto offset_reg = GetNewReg(INT64);
    I32toReg(offset_reg, 0);
    auto res = (RegOperand *)ins->GetResult();
    auto res_reg = GetIRReg(res->GetRegNo(), INT64);
    if(flag){
        for (int i = 0; i < ins->GetIndexes().size(); i++) {
            if(i!=0){
                size/=ins->GetDims()[i-1]; 
            }
            Register cur_index_reg;
            if (ins->GetIndexes()[i]->GetOperandType() == BasicOperand::IMMI32) {
                auto imm = (ImmI32Operand*)ins->GetIndexes()[i];
                cur_index_reg=GetNewReg(INT64);
                I32toReg(cur_index_reg,imm->GetIntImmVal());
            } else {//REG
                auto reg = (RegOperand*)ins->GetIndexes()[i];
                cur_index_reg=GetIRReg(reg->GetRegNo(),INT64);
            }
            Register cur_offset_reg=GetNewReg(INT64);
            I32toReg(cur_offset_reg,size);
            Register mid_reg=GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructR(RISCV_MULW, mid_reg, cur_offset_reg, cur_index_reg));
            cur_block->push_back(rvconstructor->ConstructR(RISCV_ADDW, offset_reg, mid_reg, offset_reg));
        }
        cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLLI, offset_reg, offset_reg, 2));
    }else{
        for (int i = 1; i < ins->GetIndexes().size(); i++) {
            size/=ins->GetDims()[i-1]; 
            Register cur_index_reg;
            if (ins->GetIndexes()[i]->GetOperandType() == BasicOperand::IMMI32) {
                auto imm = (ImmI32Operand*)ins->GetIndexes()[i];
                cur_index_reg=GetNewReg(INT64);
                I32toReg(cur_index_reg,imm->GetIntImmVal());
            } else {//REG
                auto reg = (RegOperand*)ins->GetIndexes()[i];
                cur_index_reg=GetIRReg(reg->GetRegNo(),INT64);
            }
            Register cur_offset_reg=GetNewReg(INT64);
            I32toReg(cur_offset_reg,size);
            Register mid_reg=GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructR(RISCV_MULW, mid_reg, cur_offset_reg, cur_index_reg));
            cur_block->push_back(rvconstructor->ConstructR(RISCV_ADDW, offset_reg, mid_reg, offset_reg));
        }
        cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLLI, offset_reg, offset_reg, 2));
    }
    if (ins->GetPtrVal()->GetOperandType() == BasicOperand::REG) {
        auto ptr = (RegOperand *)ins->GetPtrVal();
        Register base_reg;
        if(alloca_map.find(ptr->GetRegNo())==alloca_map.end()){
            base_reg=GetIRReg(ptr->GetRegNo(), INT64);
        }else{
            auto sp_offset = alloca_map[ptr->GetRegNo()];
            base_reg = GetNewReg(INT64);
            auto rv_ins = rvconstructor->ConstructIImm(RISCV_ADDI, base_reg, GetPhysicalReg(RISCV_sp), sp_offset);
            ((RiscV64Function *)cur_func)->use_stack_ins.push_back(rv_ins);
            cur_block->push_back(rv_ins);
        }
        cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD,res_reg, base_reg, offset_reg));
    } else if (ins->GetPtrVal()->GetOperandType() == BasicOperand::GLOBAL) {
        auto global = (GlobalOperand *)ins->GetPtrVal();
        auto base_reg = GetNewReg(INT64);
        cur_block->push_back(rvconstructor->ConstructULabel(RISCV_LUI, base_reg, RiscVLabel(global->GetName(), true)));
        cur_block->push_back(rvconstructor->ConstructILabel(RISCV_ADDI, base_reg, base_reg, RiscVLabel(global->GetName(), false)));
        cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD,res_reg, base_reg, offset_reg));
    }
}
template <> void RiscV64Selector::ConvertAndAppend<PhiInstruction *>(PhiInstruction *ins) {
    auto res = (RegOperand *)ins->GetResult();
    Register result_reg;
    Register phi_reg;
    if (ins->GetResultType()== BasicInstruction::LLVMType::I32 || 
        ins->GetResultType()== BasicInstruction::LLVMType::PTR) {
        result_reg = GetIRReg(res->GetRegNo(), INT64);
        phi_reg = GetNewReg(INT64);
    } else {
        result_reg = GetIRReg(res->GetRegNo(), FLOAT64);
        phi_reg = GetNewReg(FLOAT64);
    }
    auto phi = new MachinePhiInstruction(result_reg);
    
    phi_merge[cur_block->getLabelId()][result_reg.reg_no] = phi_reg;
    for (auto [label, val] : ins->GetPhiList()) {
        auto label_op= (LabelOperand *)label;
        auto reg_op = (RegOperand *)val;
        Register val_reg;
        if(ins->GetResultType()==BasicInstruction::LLVMType::FLOAT32){
            val_reg= GetIRReg(reg_op->GetRegNo(), FLOAT64);
            phi->pushPhiList(label_op->GetLabelNo(), val_reg);
        }else{
            val_reg = GetIRReg(reg_op->GetRegNo(), INT64);
            phi->pushPhiList(label_op->GetLabelNo(), val_reg);
        }
        phi_split[label_op->GetLabelNo()][phi_reg] = val_reg;
    }
    cur_block->push_back(phi);
}
//总的选择函数
template <> void RiscV64Selector::ConvertAndAppend<Instruction>(Instruction inst) {
    switch (inst->GetOpcode()) {
        case BasicInstruction::LOAD:
            ConvertAndAppend<LoadInstruction *>((LoadInstruction *)inst);
            break;
        case BasicInstruction::STORE:
            ConvertAndAppend<StoreInstruction *>((StoreInstruction *)inst);
            break;
        case BasicInstruction::ADD:
        case BasicInstruction::SUB:
        case BasicInstruction::MUL:
        case BasicInstruction::DIV:
        case BasicInstruction::FADD:
        case BasicInstruction::FSUB:
        case BasicInstruction::FMUL:
        case BasicInstruction::FDIV:
        case BasicInstruction::MOD:
        case BasicInstruction::SHL:
        case BasicInstruction::BITXOR:
            ConvertAndAppend<ArithmeticInstruction *>((ArithmeticInstruction *)inst);
            break;
        case BasicInstruction::ICMP:
            ConvertAndAppend<IcmpInstruction *>((IcmpInstruction *)inst);
            break;
        case BasicInstruction::FCMP:
            ConvertAndAppend<FcmpInstruction *>((FcmpInstruction *)inst);
            break;
        case BasicInstruction::ALLOCA:
            ConvertAndAppend<AllocaInstruction *>((AllocaInstruction *)inst);
            break;
        case BasicInstruction::BR_COND:
            ConvertAndAppend<BrCondInstruction *>((BrCondInstruction *)inst);
            break;
        case BasicInstruction::BR_UNCOND:
            ConvertAndAppend<BrUncondInstruction *>((BrUncondInstruction *)inst);
            break;
        case BasicInstruction::RET:
            ConvertAndAppend<RetInstruction *>((RetInstruction *)inst);
            break;
        case BasicInstruction::ZEXT:
            ConvertAndAppend<ZextInstruction *>((ZextInstruction *)inst);
            break;
        case BasicInstruction::FPTOSI:
            ConvertAndAppend<FptosiInstruction *>((FptosiInstruction *)inst);
            break;
        case BasicInstruction::SITOFP:
            ConvertAndAppend<SitofpInstruction *>((SitofpInstruction *)inst);
            break;
        case BasicInstruction::GETELEMENTPTR:
            ConvertAndAppend<GetElementptrInstruction *>((GetElementptrInstruction *)inst);
            break;
        case BasicInstruction::CALL:
            ConvertAndAppend<CallInstruction *>((CallInstruction *)inst);
            break;
        case BasicInstruction::PHI:
            ConvertAndAppend<PhiInstruction *>((PhiInstruction *)inst);
            break;
        default:
            ERROR("Unknown LLVM IR instruction");
    }
}
void RiscV64Selector::PhiDestructionInCurrentFunction() {
    for(auto [block_id,merge_regs]:phi_merge){
        auto mcfg = cur_func->getMachineCFG();
        auto block = mcfg->GetNodeByBlockId(block_id)->Mblock;
        for(auto [reg_id,src]:merge_regs){
            for(auto it = block->begin();it!=block->end();it++){
                if((*it)->arch== MachineBaseInstruction::PHI){
                    auto ins = (MachinePhiInstruction *)(*it);
                    auto write_regs = ins->GetWriteReg();
                    for(auto write_reg:write_regs){
                        if(write_reg->reg_no==reg_id){
                            it = cur_block->erase(it);
                            if(src.type==INT64){
                                cur_block->insert(it, rvconstructor->ConstructR(RISCV_ADD,*write_reg,src,GetPhysicalReg(RISCV_x0)));
                            }else{
                                cur_block->insert(it, rvconstructor->ConstructR2(RISCV_FMV_S, *write_reg,src));
                            }
                        }
                    }
                }
            }
        }
    }

    for(auto [label_id,split_regs]:phi_split){
        auto mcfg = cur_func->getMachineCFG();
        auto block = mcfg->GetNodeByBlockId(label_id)->Mblock;
        auto it = block->end();
        cur_block = block;
        it--;
        if(it!=block->begin()){
            it--;
            auto ins = (RiscV64Instruction *)(*it);
            if(ins-> getOpcode() == RISCV_BNE ||ins-> getOpcode() == RISCV_BEQ ||
                ins->getOpcode()==RISCV_BGE ||ins->getOpcode() == RISCV_BGT||
                ins->getOpcode()==RISCV_BLE ||ins->getOpcode() == RISCV_BLT||
                ins->getOpcode()==RISCV_BGEU ||ins->getOpcode() == RISCV_BLEU||
                ins->getOpcode()==RISCV_BGTU ||ins->getOpcode() == RISCV_BLTU){

            }else{
                it++;
            }
                
        }
        for(auto [res_reg,val_reg]:split_regs){
            if(res_reg.type==INT64){
                cur_block->insert(it, rvconstructor->ConstructR(RISCV_ADD,res_reg,val_reg, GetPhysicalReg(RISCV_x0)));
            }else{
                cur_block->insert(it, rvconstructor->ConstructR2(RISCV_FMV_S, res_reg, val_reg));
            } 
        }
    }
}
void RiscV64Selector::SelectInstructionAndBuildCFG() {
    // 与中间代码生成一样, 如果你完全无从下手, 可以先看看输出是怎么写的
    // 即riscv64gc/instruction_print/*  common/machine_passes/machine_printer.h

    // 指令选择除了一些函数调用约定必须遵守的情况需要物理寄存器，其余情况必须均为虚拟寄存器
    dest->global_def = IR->global_def;//全局保留
    // 遍历每个LLVM IR函数
    for (auto [defI,cfg] : IR->llvm_cfg) {
        if(cfg == nullptr){
            ERROR("LLVMIR CFG is Empty, you should implement BuildCFG in MidEnd first");
        }
        std::string name = cfg->function_def->GetFunctionName();

        cur_func = new RiscV64Function(name);
        cur_func->SetParent(dest);
        // 你可以使用cur_func->GetNewRegister来获取新的虚拟寄存器
        dest->functions.push_back(cur_func);

        auto cur_mcfg = new MachineCFG;
        cur_func->SetMachineCFG(cur_mcfg);

        // 清空指令选择状态(可能需要自行添加初始化操作)
        ClearFunctionSelectState();
        // 添加函数参数
        MachineDataType f_type;
        int cnt=0;
        for (auto it:defI->formals){
            int flag = false;
            if(it == BasicInstruction::LLVMType::I32){
                f_type = INT64;
            }else if (it == BasicInstruction::LLVMType::FLOAT32) {
                f_type = FLOAT64;
            }else if (it == BasicInstruction::LLVMType::PTR){
                f_type = INT64;
                flag=true;
            }
            
            auto formal_reg = (RegOperand*)defI->formals_reg[cnt++];
            cur_func->AddParameter(GetIRReg(formal_reg->GetRegNo(), f_type));
            if(flag){
                array_select[formal_reg->GetRegNo()]=true;
            }
        }
        // 遍历每个LLVM IR基本块
        for (auto [id, block] : *(cfg->block_map)) {
            cur_block = new RiscV64Block(id);
            // 将新块添加到Machine CFG中
            cur_mcfg->AssignEmptyNode(id, cur_block);
            cur_func->UpdateMaxLabel(id);

            cur_block->setParent(cur_func);
            cur_func->blocks.push_back(cur_block);

            // 指令选择主要函数, 请注意指令选择时需要维护变量cur_offset
            for (auto instruction : block->Instruction_list) {
                ConvertAndAppend<Instruction>(instruction);
            }
        }
        // RISCV 8字节对齐（）
        if (cur_offset % 8 != 0) {
            cur_offset = ((cur_offset + 7) / 8) * 8;
        }
        //新栈=指令分配后的局部变量使用栈+调用其他函数的额外参数使用栈。
        cur_func->SetStackSize(cur_offset + cur_func->GetParaSize());
        ((RiscV64Function*)cur_func)->UpdatePara_In_UseStack();
        PhiDestructionInCurrentFunction();//消除PHI！
        // 控制流图连边
        for (int i = 0; i < cfg->G.size(); i++) {
            const auto &arcs = cfg->G[i];
            for (auto arc : arcs) {
                cur_mcfg->MakeEdge(i, arc->block_id);
            }
        }
    }
    
}
void RiscV64Selector::ClearFunctionSelectState() { 
    cur_offset = 0; 
    reg_map.clear();
    alloca_map.clear();
    cmp_map.clear();
    array_select.clear();
    phi_split.clear();
    phi_merge.clear();
}