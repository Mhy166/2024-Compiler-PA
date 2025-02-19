#include "../include/Instruction.h"
#include "../include/basic_block.h"
#include <assert.h>
#include <unordered_map>

static std::unordered_map<int, RegOperand *> RegOperandMap;
static std::map<int, LabelOperand *> LabelOperandMap;
static std::map<std::string, GlobalOperand *> GlobalOperandMap;

RegOperand *GetNewRegOperand(int RegNo) {
    auto it = RegOperandMap.find(RegNo);
    if (it == RegOperandMap.end()) {
        auto R = new RegOperand(RegNo);
        RegOperandMap[RegNo] = R;
        return R;
    } else {
        return it->second;
    }
}

LabelOperand *GetNewLabelOperand(int LabelNo) {
    auto it = LabelOperandMap.find(LabelNo);
    if (it == LabelOperandMap.end()) {
        auto L = new LabelOperand(LabelNo);
        LabelOperandMap[LabelNo] = L;
        return L;
    } else {
        return it->second;
    }
}

GlobalOperand *GetNewGlobalOperand(std::string name) {
    auto it = GlobalOperandMap.find(name);
    if (it == GlobalOperandMap.end()) {
        auto G = new GlobalOperand(name);
        GlobalOperandMap[name] = G;
        return G;
    } else {
        return it->second;
    }
}


//上面三个函数是操作数生成的，给标号就可以。下面是所有IR指令的接口了，我们要生成的话，就调用下面这些。

void IRgenArithmeticI32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1, new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::I32, GetNewRegOperand(reg1),
                                                      GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1,
                         new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(reg1),
                                                   GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticI32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int reg2, int result_reg) {
    B->InsertInstruction(1, new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::I32, new ImmI32Operand(val1),
                                                      GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, int reg2,
                               int result_reg) {
    B->InsertInstruction(1,
                         new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(val1),
                                                   GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticI32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int val2, int result_reg) {
    B->InsertInstruction(1, new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::I32, new ImmI32Operand(val1),
                                                      new ImmI32Operand(val2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, float val2,
                              int result_reg) {
    B->InsertInstruction(1,
                         new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(val1),
                                                   new ImmF32Operand(val2), GetNewRegOperand(result_reg)));
}
//整数比较
void IRgenIcmp(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1, new IcmpInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(reg1),
                                                GetNewRegOperand(reg2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenFcmp(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1, new FcmpInstruction(BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(reg1),
                                                GetNewRegOperand(reg2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenIcmpImmRight(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int val2, int result_reg) {
    B->InsertInstruction(1, new IcmpInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(reg1),
                                                new ImmI32Operand(val2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenFcmpImmRight(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, float val2, int result_reg) {
    B->InsertInstruction(1, new FcmpInstruction(BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(reg1),
                                                new ImmF32Operand(val2), cmp_op, GetNewRegOperand(result_reg)));
}
//类型转换
//float->int
void IRgenFptosi(LLVMBlock B, int src, int dst) {
    B->InsertInstruction(1, new FptosiInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
}

//float<-int
void IRgenSitofp(LLVMBlock B, int src, int dst) {
    B->InsertInstruction(1, new SitofpInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
}
////bool->int
void IRgenZextI1toI32(LLVMBlock B, int src, int dst) {
    B->InsertInstruction(1, new ZextInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(dst),
                                                BasicInstruction::LLVMType::I1, GetNewRegOperand(src)));
}
//索引
void IRgenGetElementptrIndexI32(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs) {
    B->InsertInstruction(1, new GetElementptrInstruction(type, GetNewRegOperand(result_reg), ptr, dims, indexs, BasicInstruction::I32));
}

void IRgenGetElementptrIndexI64(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs) {
    B->InsertInstruction(1, new GetElementptrInstruction(type, GetNewRegOperand(result_reg), ptr, dims, indexs, BasicInstruction::I64));
}
//内存指令
void IRgenLoad(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr) {
    B->InsertInstruction(1, new LoadInstruction(type, ptr, GetNewRegOperand(result_reg)));
}

void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, int value_reg, Operand ptr) {
    B->InsertInstruction(1, new StoreInstruction(type, ptr, GetNewRegOperand(value_reg)));
}

void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, Operand value, Operand ptr) {
    B->InsertInstruction(1, new StoreInstruction(type, ptr, value));
}
//调用指令
void IRgenCall(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg,
               std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(result_reg), name, args));
}

void IRgenCallVoid(LLVMBlock B, BasicInstruction::LLVMType type,
                   std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(-1), name, args));
}

void IRgenCallNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(result_reg), name));
}

void IRgenCallVoidNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(-1), name));
}
//返回
void IRgenRetReg(LLVMBlock B, BasicInstruction::LLVMType type, int reg) {
    B->InsertInstruction(1, new RetInstruction(type, GetNewRegOperand(reg)));
}

void IRgenRetImmInt(LLVMBlock B, BasicInstruction::LLVMType type, int val) {
    B->InsertInstruction(1, new RetInstruction(type, new ImmI32Operand(val)));
}

void IRgenRetImmFloat(LLVMBlock B, BasicInstruction::LLVMType type, float val) {
    B->InsertInstruction(1, new RetInstruction(type, new ImmF32Operand(val)));
}

void IRgenRetVoid(LLVMBlock B) {
    B->InsertInstruction(1, new RetInstruction(BasicInstruction::LLVMType::VOID, nullptr));
}
//跳转
void IRgenBRUnCond(LLVMBlock B, int dst_label) {
    B->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(dst_label)));
}

void IRgenBrCond(LLVMBlock B, int cond_reg, int true_label, int false_label) {
    B->InsertInstruction(1, new BrCondInstruction(GetNewRegOperand(cond_reg), GetNewLabelOperand(true_label),
                                                  GetNewLabelOperand(false_label)));
}
//分配
void IRgenAlloca(LLVMBlock B, BasicInstruction::LLVMType type, int reg) {
    B->InsertInstruction(0, new AllocaInstruction(type, GetNewRegOperand(reg)));
}

void IRgenAllocaArray(LLVMBlock B, BasicInstruction::LLVMType type, int reg, std::vector<int> dims) {
    B->InsertInstruction(0, new AllocaInstruction(type, dims, GetNewRegOperand(reg)));
}


//换名函数
void ArithmeticInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if(GetOperand1()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetOperand1())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetOperand1(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetOperand1())->GetRegNo();
        }
    }
    if(GetOperand2()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetOperand2())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetOperand2(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetOperand2())->GetRegNo();
        }
    }
    if (GetResult()->GetOperandType() == BasicOperand::REG) {
        int reg=((RegOperand*)GetResult())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetResultReg(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetResult())->GetRegNo();
        }
    }
    
}
void IcmpInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if(GetOp1()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetOp1())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetOp1(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetOp1())->GetRegNo();
        }
    }
    if(GetOp2()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetOp2())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetOp2(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetOp2())->GetRegNo();
        }
    }
    if (GetResult()->GetOperandType() == BasicOperand::REG) {
        int reg=((RegOperand*)GetResult())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetResultReg(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetResult())->GetRegNo();
        }
    }
    
}
void FcmpInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if(GetOp1()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetOp1())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetOp1(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetOp1())->GetRegNo();
        }
    }
    if(GetOp2()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetOp2())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetOp2(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetOp2())->GetRegNo();
        }
    }
    if (GetResult()->GetOperandType() == BasicOperand::REG) {
        int reg=((RegOperand*)GetResult())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetResultReg(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetResult())->GetRegNo();
        }
    }
    

}
void PhiInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
  
    for (auto &label_pair : phi_list) {
        auto &op1 = label_pair.first;
        if (op1->GetOperandType() == BasicOperand::REG) {
            auto op1_reg = (RegOperand *)op1;
            if (mem2reg_map.find(op1_reg->GetRegNo()) != mem2reg_map.end())
                op1 = GetNewRegOperand(mem2reg_map.find(op1_reg->GetRegNo())->second);
        }
        auto &op2 = label_pair.second;
        if (op2->GetOperandType() == BasicOperand::REG) {
            auto op2_reg = (RegOperand *)op2;
            if (mem2reg_map.find(op2_reg->GetRegNo()) != mem2reg_map.end())
                op2 = GetNewRegOperand(mem2reg_map.find(op2_reg->GetRegNo())->second);
        }
    }
    if (GetResult()->GetOperandType() == BasicOperand::REG) {
        int reg=((RegOperand*)GetResult())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetResultReg(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetResult())->GetRegNo();
        }
    }
}
void AllocaInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if (result->GetOperandType() == BasicOperand::operand_type::REG) {
        int reg = ((RegOperand*)&result)->GetRegNo();        
        // 循环查找并更新寄存器编号
        while (mem2reg_map.find(reg) != mem2reg_map.end()) {
            // 根据映射表更新 result 寄存器
            reg = mem2reg_map[reg];
            result = GetNewRegOperand(reg); // 使用新寄存器创建新的 RegOperand
        }
    }
    
}
void BrCondInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if(GetCond()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetCond())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetCond(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetCond())->GetRegNo();
        }
    }
    
}

void BrUncondInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
}
void GlobalVarDefineInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
}
void GlobalStringConstInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
}
void CallInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
   
    for (auto &arg_pair : args) {
        if (arg_pair.second->GetOperandType() == BasicOperand::REG) {
            auto op = (RegOperand *)arg_pair.second;
            if (mem2reg_map.find(op->GetRegNo()) != mem2reg_map.end())
                arg_pair.second = GetNewRegOperand(mem2reg_map.find(op->GetRegNo())->second);
        }
    }
    if (result != NULL) {
        if (result->GetOperandType() == BasicOperand::REG) {
            auto result_reg = (RegOperand *)result;
            if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
                this->result = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
        }
    }
}
void RetInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if (ret_val != NULL) {
        if (ret_val->GetOperandType() == BasicOperand::REG) {
            auto result_reg = (RegOperand *)ret_val;
            if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
                ret_val = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
        }
    }
}
void GetElementptrInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
   
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            this->result = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
    if (ptrval->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)ptrval;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            this->ptrval = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
    for (auto &idx_pair : indexes) {
        if (idx_pair->GetOperandType() == BasicOperand::REG) {
            auto idx_reg = (RegOperand *)idx_pair;
            if (mem2reg_map.find(idx_reg->GetRegNo()) != mem2reg_map.end())
                idx_pair = GetNewRegOperand(mem2reg_map.find(idx_reg->GetRegNo())->second);
        }
    }
}
//这俩不用换名
void FunctionDefineInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
}
void FunctionDeclareInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
}
void FptosiInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
   
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            this->result = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)value;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            value = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
}
void SitofpInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            this->result = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)value;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            value = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
}
void ZextInstruction::Reg_Rename(std::map<int,int> mem2reg_map){

    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            this->result = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)value;
        if (mem2reg_map.find(result_reg->GetRegNo()) != mem2reg_map.end())
            value = GetNewRegOperand(mem2reg_map.find(result_reg->GetRegNo())->second);
    }
}
void StoreInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if(GetPointer()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetPointer())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetPointer(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetPointer())->GetRegNo();
        }
    }
    if(GetValue()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetValue())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetValue(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetValue())->GetRegNo();
        }
    }

}
void LoadInstruction::Reg_Rename(std::map<int,int> mem2reg_map){
    if(GetPointer()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetPointer())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetPointer(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetPointer())->GetRegNo();
        }
    }
    if(GetResult()->GetOperandType()==BasicOperand::operand_type::REG){
        int reg=((RegOperand*)GetResult())->GetRegNo();
        while(mem2reg_map.find(reg)!=mem2reg_map.end()){
            SetResult(GetNewRegOperand(mem2reg_map[reg]));
            reg=((RegOperand*)GetResult())->GetRegNo();
        }
    }
}
//标签换名
void PhiInstruction::Label_Rename(std::map<int, int> label_map) {
    for (auto &[label, val] : phi_list) {
        auto l = (LabelOperand *)label;
        if (label_map.find(l->GetLabelNo()) != label_map.end()) {
            label = GetNewLabelOperand(label_map.find(l->GetLabelNo())->second);
        }
    }
}

void BrCondInstruction::Label_Rename(std::map<int, int> label_map) {
    auto true_label = (LabelOperand *)this->trueLabel;
    auto false_label = (LabelOperand *)this->falseLabel;

    if (label_map.find(true_label->GetLabelNo()) != label_map.end()) {
        trueLabel = GetNewLabelOperand(label_map.find(true_label->GetLabelNo())->second);
    }

    if (label_map.find(false_label->GetLabelNo()) != label_map.end()) {
        falseLabel = GetNewLabelOperand(label_map.find(false_label->GetLabelNo())->second);
    }
}

void BrUncondInstruction::Label_Rename(std::map<int, int> label_map) {
    auto dest_label = (LabelOperand *)this->destLabel;

    if (label_map.find(dest_label->GetLabelNo()) != label_map.end()) {
        destLabel = GetNewLabelOperand(label_map.find(dest_label->GetLabelNo())->second);
    }
}

//复制操作数
Operand RegOperand::CopyOperand() { return GetNewRegOperand(reg_no); }

Operand ImmI32Operand::CopyOperand() { return new ImmI32Operand(immVal); }

Operand ImmF32Operand::CopyOperand() { return new ImmF32Operand(immVal); }

Operand ImmI64Operand::CopyOperand() { return new ImmI64Operand(immVal); }

Operand LabelOperand::CopyOperand() { return GetNewLabelOperand(label_no); }

Operand GlobalOperand::CopyOperand() { return GetNewGlobalOperand(name); }

//复制函数
Instruction LoadInstruction::CopyInstruction() {
    Operand npointer = pointer->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new LoadInstruction(type, npointer, nresult);
}

Instruction StoreInstruction::CopyInstruction() {
    Operand npointer = pointer->CopyOperand();
    Operand nvalue = value->CopyOperand();
    return new StoreInstruction(type, npointer, nvalue);
}

Instruction ArithmeticInstruction::CopyInstruction() {

    Operand nop1 = op1->CopyOperand();
    Operand nop2 = op2->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new ArithmeticInstruction(opcode, type, nop1, nop2, nresult);
}

Instruction IcmpInstruction::CopyInstruction() {
    Operand nop1 = op1->CopyOperand();
    Operand nop2 = op2->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new IcmpInstruction(type, nop1, nop2, cond, nresult);
}

Instruction FcmpInstruction::CopyInstruction() {
    Operand nop1 = op1->CopyOperand();
    Operand nop2 = op2->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new FcmpInstruction(type, nop1, nop2, cond, nresult);
}

Instruction PhiInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    std::vector<std::pair<Operand, Operand>> nval_labels;
    for (auto Phiop : phi_list) {
        Operand nlabel = Phiop.first->CopyOperand();
        Operand nvalue = Phiop.second->CopyOperand();
        nval_labels.push_back(std::make_pair(nlabel, nvalue));
    }

    return new PhiInstruction(type, nresult, nval_labels);
}

Instruction AllocaInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    std::vector<int> ndims;
    for (auto dimint : dims) {
        ndims.push_back(dimint);
    }
    return new AllocaInstruction(type, ndims, nresult);
}

Instruction BrCondInstruction::CopyInstruction() {
    Operand ncond = cond->CopyOperand();
    Operand ntrueLabel = trueLabel->CopyOperand();
    Operand nfalseLabel = falseLabel->CopyOperand();
    return new BrCondInstruction(ncond, ntrueLabel, nfalseLabel);
}

Instruction BrUncondInstruction::CopyInstruction() {
    Operand ndestLabel = destLabel->CopyOperand();
    return new BrUncondInstruction(ndestLabel);
}

Instruction CallInstruction::CopyInstruction() {
    Operand nresult = NULL;
    if (ret_type != VOID) {
        nresult = result->CopyOperand();
    }
    std::vector<std::pair<enum LLVMType, Operand>> nargs;
    for (auto n : args) {
        nargs.push_back({n.first, n.second->CopyOperand()});
    }
    return new CallInstruction(ret_type, nresult, name, nargs);
}

Instruction RetInstruction::CopyInstruction() { return new RetInstruction(ret_type, ret_val); }

Instruction GetElementptrInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nptrval = ptrval->CopyOperand();
    std::vector<Operand> nindexes;
    for (auto index : indexes) {
        Operand nindex = index->CopyOperand();
        nindexes.push_back(nindex);
    }

    return new GetElementptrInstruction(type, nresult, nptrval, dims, nindexes,index_type);
}

Instruction FptosiInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nvalue = value->CopyOperand();

    return new FptosiInstruction(nresult, nvalue);
}
Instruction SitofpInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nvalue = value->CopyOperand();

    return new SitofpInstruction(nresult, nvalue);
}

Instruction ZextInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nvalue = value->CopyOperand();

    return new ZextInstruction(to_type, nresult, from_type, nvalue);
}
