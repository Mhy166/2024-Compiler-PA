#include "IRgen.h"
#include "../include/ir.h"
#include "semant.h"
#include "../include/Instruction.h"

extern SemantTable semant_table;    // 也许你会需要一些语义分析的信息

IRgenTable irgen_table;    // 中间代码生成的辅助变量
LLVMIR llvmIR;             // 我们需要在这个变量中生成中间代码
int now_label=0;
FuncDefInstruction now_func;
BasicInstruction::LLVMType now_retty;
BasicInstruction::LLVMType ToLT[6]={
    BasicInstruction::LLVMType::VOID,
    BasicInstruction::LLVMType::I32,
    BasicInstruction::LLVMType::FLOAT32,
    BasicInstruction::LLVMType::I1,
    BasicInstruction::LLVMType::PTR,
    BasicInstruction::LLVMType::DOUBLE
};

int top_reg=-1;
int top_label=-1;
int while_cond_label=-1;
int while_end_label=-1;
void AddLibFunctionDeclare();

// 在基本块B末尾生成一条新指令
void IRgenArithmeticI32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg);
void IRgenArithmeticF32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg);
void IRgenArithmeticI32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int reg2, int result_reg);
void IRgenArithmeticF32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, int reg2,
                               int result_reg);
void IRgenArithmeticI32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int val2, int result_reg);
void IRgenArithmeticF32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, float val2,
                              int result_reg);

void IRgenIcmp(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int reg2, int result_reg);
void IRgenFcmp(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, int reg2, int result_reg);
void IRgenIcmpImmRight(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int val2, int result_reg);
void IRgenFcmpImmRight(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, float val2, int result_reg);

void IRgenFptosi(LLVMBlock B, int src, int dst);
void IRgenSitofp(LLVMBlock B, int src, int dst);
void IRgenZextI1toI32(LLVMBlock B, int src, int dst);

void IRgenGetElementptrIndexI32(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs);

void IRgenGetElementptrIndexI64(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs);

void IRgenLoad(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr);
void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, int value_reg, Operand ptr);
void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, Operand value, Operand ptr);

void IRgenCall(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg,
               std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name);
void IRgenCallVoid(LLVMBlock B, BasicInstruction::LLVMType type,
                   std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name);

void IRgenCallNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, std::string name);
void IRgenCallVoidNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, std::string name);

void IRgenRetReg(LLVMBlock B, BasicInstruction::LLVMType type, int reg);
void IRgenRetImmInt(LLVMBlock B, BasicInstruction::LLVMType type, int val);
void IRgenRetImmFloat(LLVMBlock B, BasicInstruction::LLVMType type, float val);
void IRgenRetVoid(LLVMBlock B);

void IRgenBRUnCond(LLVMBlock B, int dst_label);
void IRgenBrCond(LLVMBlock B, int cond_reg, int true_label, int false_label);

void IRgenAlloca(LLVMBlock B, BasicInstruction::LLVMType type, int reg);
void IRgenAllocaArray(LLVMBlock B, BasicInstruction::LLVMType type, int reg, std::vector<int> dims);

RegOperand *GetNewRegOperand(int RegNo);

// generate TypeConverse Instructions from type_src to type_dst
// eg. you can use fptosi instruction to converse float to int
// eg. you can use zext instruction to converse bool to int
void IRgenTypeConverse(LLVMBlock B, Type::ty type_src, Type::ty type_dst, int src, int dst) {
    TODO("IRgenTypeConverse. Implement it if you need it");
}

void BasicBlock::InsertInstruction(int pos, Instruction Ins) {
    assert(pos == 0 || pos == 1);
    if (pos == 0) {
        Instruction_list.push_front(Ins);
    } else if (pos == 1) {
        Instruction_list.push_back(Ins);
    }
}

/*
二元运算指令生成的伪代码：
    假设现在的语法树节点是：AddExp_plus
    该语法树表示 addexp + mulexp

    addexp->codeIR()
    mulexp->codeIR()
    假设mulexp生成完后，我们应该在基本块B0继续插入指令。
    addexp的结果存储在r0寄存器中，mulexp的结果存储在r1寄存器中
    生成一条指令r2 = r0 + r1，并将该指令插入基本块B0末尾。
    标注后续应该在基本块B0插入指令，当前节点的结果寄存器为r2。
    (如果考虑支持浮点数，需要查看语法树节点的类型来判断此时是否需要隐式类型转换)
*/


int Bin_ToInt(LLVMBlock b,int src,Type::ty type){
    if(type==Type::INT){
        return src;
    }else if(type==Type::FLOAT){
        IRgenFptosi(b,src,++top_reg);
    }
    return top_reg;
}
int Bin_ToFloat(LLVMBlock b,int src,Type::ty type){
    if(type==Type::FLOAT){
        return src;
    }else if(type==Type::INT){
        IRgenSitofp(b,src,++top_reg);
    }else if(type==Type::BOOL){
        IRgenSitofp(b,src,++top_reg);
    }
    return top_reg;
}
int Bin_ToBool(LLVMBlock b,int src,Type::ty type){
    if(type==Type::FLOAT){
        IRgenFcmpImmRight(b,BasicInstruction::FcmpCond::OEQ,src,0,++top_reg);
    }else{
        IRgenIcmpImmRight(b,BasicInstruction::IcmpCond::eq,src,0,++top_reg);
    }
    return top_reg;
}
void AddRet(LLVMBlock b,Type::ty type){
    if(type==Type::VOID){
        IRgenRetVoid(b);
    }else if(type==Type::INT){
        IRgenRetImmInt(b,BasicInstruction::LLVMType::I32,0);
    }else if(type==Type::FLOAT){
        IRgenRetImmFloat(b,BasicInstruction::LLVMType::FLOAT32,0);
    }
}

std::vector<Operand> ToIndex(std::vector<int> dims,int i){
    std::vector<Operand> res;
    res.push_back(new ImmI32Operand(0));
    int size=1;
    for(auto i:dims){
        size*=i;
    }
    
    int cnt=0;
    while(cnt!=dims.size()){
        size/=dims[cnt++];
        res.push_back(new ImmI32Operand(i/size));
        i%=size;
    }
    return res;
}

void InitIntFloat(std::vector<int> dims,InitVal init,int begin,int reg_ptr,std::vector<int> origin_dims,Type::ty type){
    //进入一个初始化器
    int idx=0;
    int res_size=1;
    for(auto i:dims){
        res_size*=i;
    }
    std::vector<InitVal>* initvals;
    if(init->IsConst()){
        initvals=((ConstInitVal*)init)->initval;
    }else{
        initvals=((VarInitVal*)init)->initval;
    }
    for(auto i:*initvals){
        if(i->Exp()){
            LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
            IRgenGetElementptrIndexI32(b,ToLT[type],++top_reg,
                GetNewRegOperand(reg_ptr),origin_dims,ToIndex(origin_dims,idx+begin));
            int src=top_reg;
            i->codeIR();
            int init_reg=top_reg;
            if(type==Type::INT){
                Bin_ToInt(b,init_reg,(i->GetExp()->GetNodeAttribute().T.type));
            }else if(type==Type::FLOAT){
                Bin_ToFloat(b,init_reg,(i->GetExp()->GetNodeAttribute().T.type));
            }
            IRgenStore(b,ToLT[type],top_reg,GetNewRegOperand(src));
            idx++;
        }else{
            //碰到新的初始化器
            int dim_scope=1;//下一个初始化器负责的维数
            int num_scope=res_size/dims[0];//下一个初始化器负责的数量
            std::vector<int> new_dims;
            while(idx%num_scope!=0){
                num_scope/=dims[dim_scope];
                dim_scope++;
            }
            for(int t=dim_scope;t<dims.size();t++){
                new_dims.push_back(dims[t]);
            }
            InitIntFloat(new_dims,i,begin+idx,reg_ptr,origin_dims,type);
            idx+=num_scope;
        }
    }
}



//√————————————————————————————————
void __Program::codeIR() {
    AddLibFunctionDeclare();
    auto comp_vector = *comp_list;
    for (auto comp : comp_vector) {
        comp->codeIR();
    }
}
void Exp::codeIR() { addexp->codeIR(); }
void AddExp_plus::codeIR() {
    addexp->codeIR();
    int reg1=top_reg;
    mulexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    //类型转换，这里的目标类型只有INT和FLOAT，而且在该父节点已经标好（类型检查做的）
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,addexp->GetNodeAttribute().T.type);
        reg2=Bin_ToInt(b,reg2,mulexp->GetNodeAttribute().T.type);
        IRgenArithmeticI32(b,BasicInstruction::LLVMIROpcode::ADD,reg1,reg2,++top_reg);
    }else if(GetNodeAttribute().T.type==Type::FLOAT){
        reg1=Bin_ToFloat(b,reg1,addexp->GetNodeAttribute().T.type);
        reg2=Bin_ToFloat(b,reg2,mulexp->GetNodeAttribute().T.type);
        IRgenArithmeticF32(b,BasicInstruction::LLVMIROpcode::FADD,reg1,reg2,++top_reg);
    }
}
void AddExp_sub::codeIR() {
    addexp->codeIR();
    int reg1=top_reg;
    mulexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,addexp->GetNodeAttribute().T.type);
        reg2=Bin_ToInt(b,reg2,mulexp->GetNodeAttribute().T.type);
        IRgenArithmeticI32(b,BasicInstruction::LLVMIROpcode::SUB,reg1,reg2,++top_reg);
    }else if(GetNodeAttribute().T.type==Type::FLOAT){
        reg1=Bin_ToFloat(b,reg1,addexp->GetNodeAttribute().T.type);
        reg2=Bin_ToFloat(b,reg2,mulexp->GetNodeAttribute().T.type);
        IRgenArithmeticF32(b,BasicInstruction::LLVMIROpcode::FSUB,reg1,reg2,++top_reg);
    }
}
void MulExp_mul::codeIR() {
    mulexp->codeIR();
    int reg1=top_reg;
    unary_exp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,mulexp->GetNodeAttribute().T.type);
        reg2=Bin_ToInt(b,reg2,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticI32(b,BasicInstruction::LLVMIROpcode::MUL,reg1,reg2,++top_reg);
    }else if(GetNodeAttribute().T.type==Type::FLOAT){
        reg1=Bin_ToFloat(b,reg1,mulexp->GetNodeAttribute().T.type);
        reg2=Bin_ToFloat(b,reg2,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticF32(b,BasicInstruction::LLVMIROpcode::FMUL,reg1,reg2,++top_reg);
    }
}
void MulExp_div::codeIR() {
    mulexp->codeIR();
    int reg1=top_reg;
    unary_exp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,mulexp->GetNodeAttribute().T.type);
        reg2=Bin_ToInt(b,reg2,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticI32(b,BasicInstruction::LLVMIROpcode::DIV,reg1,reg2,++top_reg);
    }else if(GetNodeAttribute().T.type==Type::FLOAT){
        reg1=Bin_ToFloat(b,reg1,mulexp->GetNodeAttribute().T.type);
        reg2=Bin_ToFloat(b,reg2,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticF32(b,BasicInstruction::LLVMIROpcode::FDIV,reg1,reg2,++top_reg);
    }
}
void MulExp_mod::codeIR() { 
    mulexp->codeIR();
    int reg1=top_reg;
    unary_exp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,mulexp->GetNodeAttribute().T.type);
        reg2=Bin_ToInt(b,reg2,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticI32(b,BasicInstruction::LLVMIROpcode::MOD,reg1,reg2,++top_reg);
    }
}
void RelExp_leq::codeIR() {
    relexp->codeIR();
    int reg1=top_reg;
    addexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
  
    if(GetNodeAttribute().T.type==Type::BOOL){//目标INT
        if(relexp->GetNodeAttribute().T.type!=Type::FLOAT&&addexp->GetNodeAttribute().T.type!=Type::FLOAT){
            IRgenIcmp(b,BasicInstruction::IcmpCond::sle,reg1,reg2,++top_reg);
        }else{
            reg1=Bin_ToFloat(b,reg1,relexp->GetNodeAttribute().T.type);
            reg2=Bin_ToFloat(b,reg2,addexp->GetNodeAttribute().T.type);
            IRgenFcmp(b,BasicInstruction::FcmpCond::OLE,reg1,reg2,++top_reg);
        }
        int src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
void RelExp_lt::codeIR() {    
    relexp->codeIR();
    int reg1=top_reg;
    addexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
  
    if(GetNodeAttribute().T.type==Type::BOOL){//目标INT
        if(relexp->GetNodeAttribute().T.type!=Type::FLOAT&&addexp->GetNodeAttribute().T.type!=Type::FLOAT){
            IRgenIcmp(b,BasicInstruction::IcmpCond::slt,reg1,reg2,++top_reg);
        }else{
            reg1=Bin_ToFloat(b,reg1,relexp->GetNodeAttribute().T.type);
            reg2=Bin_ToFloat(b,reg2,addexp->GetNodeAttribute().T.type);
            IRgenFcmp(b,BasicInstruction::FcmpCond::OLT,reg1,reg2,++top_reg);
        }
        int src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
void RelExp_geq::codeIR() {
    relexp->codeIR();
    int reg1=top_reg;
    addexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
  
    if(GetNodeAttribute().T.type==Type::BOOL){//目标INT
        if(relexp->GetNodeAttribute().T.type!=Type::FLOAT&&addexp->GetNodeAttribute().T.type!=Type::FLOAT){
            IRgenIcmp(b,BasicInstruction::IcmpCond::sge,reg1,reg2,++top_reg);
        }else{
            reg1=Bin_ToFloat(b,reg1,relexp->GetNodeAttribute().T.type);
            reg2=Bin_ToFloat(b,reg2,addexp->GetNodeAttribute().T.type);
            IRgenFcmp(b,BasicInstruction::FcmpCond::OGE,reg1,reg2,++top_reg);
        }
        int src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
void RelExp_gt::codeIR() {
    relexp->codeIR();
    int reg1=top_reg;
    addexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
  
    if(GetNodeAttribute().T.type==Type::BOOL){//目标INT
        if(relexp->GetNodeAttribute().T.type!=Type::FLOAT&&addexp->GetNodeAttribute().T.type!=Type::FLOAT){
            IRgenIcmp(b,BasicInstruction::IcmpCond::sgt,reg1,reg2,++top_reg);
        }else{
            reg1=Bin_ToFloat(b,reg1,relexp->GetNodeAttribute().T.type);
            reg2=Bin_ToFloat(b,reg2,addexp->GetNodeAttribute().T.type);
            IRgenFcmp(b,BasicInstruction::FcmpCond::OGT,reg1,reg2,++top_reg);
        }
        int src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
void EqExp_eq::codeIR() {
    eqexp->codeIR();
    int reg1=top_reg;
    relexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
  
    if(GetNodeAttribute().T.type==Type::BOOL){//目标INT
        if(eqexp->GetNodeAttribute().T.type!=Type::FLOAT&&relexp->GetNodeAttribute().T.type!=Type::FLOAT){
            IRgenIcmp(b,BasicInstruction::IcmpCond::eq,reg1,reg2,++top_reg);
        }else{
            reg1=Bin_ToFloat(b,reg1,eqexp->GetNodeAttribute().T.type);
            reg2=Bin_ToFloat(b,reg2,relexp->GetNodeAttribute().T.type);
            IRgenFcmp(b,BasicInstruction::FcmpCond::OEQ,reg1,reg2,++top_reg);
        }
        int src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
void EqExp_neq::codeIR() { 
    eqexp->codeIR();
    int reg1=top_reg;
    relexp->codeIR();
    int reg2=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
  
    if(GetNodeAttribute().T.type==Type::BOOL){//目标INT
        if(eqexp->GetNodeAttribute().T.type!=Type::FLOAT&&relexp->GetNodeAttribute().T.type!=Type::FLOAT){
            IRgenIcmp(b,BasicInstruction::IcmpCond::ne,reg1,reg2,++top_reg);
        }else{
            reg1=Bin_ToFloat(b,reg1,eqexp->GetNodeAttribute().T.type);
            reg2=Bin_ToFloat(b,reg2,relexp->GetNodeAttribute().T.type);
            IRgenFcmp(b,BasicInstruction::FcmpCond::ONE,reg1,reg2,++top_reg);
        }
        int src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
// short circuit &&
void LAndExp_and::codeIR() {
    //短路的时候
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock left_true_block = llvmIR.NewBlock(now_func,top_label);
    landexp->true_label=left_true_block->block_id;
    landexp->false_label=this->false_label;
    landexp->codeIR();
    int src=top_reg;
    b = llvmIR.GetBlock(now_func, now_label);
    if(landexp->GetNodeAttribute().T.type==Type::FLOAT){
        IRgenFcmpImmRight(b,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
    }else{
        IRgenIcmpImmRight(b,BasicInstruction::IcmpCond::ne,src,0,++top_reg);
    }
    IRgenBrCond(b,top_reg,landexp->true_label,landexp->false_label);

    now_label=landexp->true_label;
    b = llvmIR.GetBlock(now_func, now_label);
    eqexp->true_label=this->true_label;
    eqexp->false_label=this->false_label;
    eqexp->codeIR();
    src=top_reg;
    b = llvmIR.GetBlock(now_func, now_label);
    if(eqexp->GetNodeAttribute().T.type==Type::FLOAT){
        IRgenFcmpImmRight(b,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
        src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
// short circuit ||
void LOrExp_or::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock left_false_block = llvmIR.NewBlock(now_func,top_label);
    lorexp->true_label=this->true_label;
    lorexp->false_label=left_false_block->block_id;
    lorexp->codeIR();
    int src=top_reg;
    b = llvmIR.GetBlock(now_func, now_label);
    if(lorexp->GetNodeAttribute().T.type==Type::FLOAT){
        IRgenFcmpImmRight(b,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
    }else{
        IRgenIcmpImmRight(b,BasicInstruction::IcmpCond::ne,src,0,++top_reg);
    }
    IRgenBrCond(b,top_reg,lorexp->true_label,lorexp->false_label);

    now_label=lorexp->false_label;
    b = llvmIR.GetBlock(now_func, now_label);
    landexp->true_label=this->true_label;
    landexp->false_label=this->false_label;
    landexp->codeIR();
    b = llvmIR.GetBlock(now_func, now_label);
    src=top_reg;
    if(landexp->GetNodeAttribute().T.type==Type::FLOAT){
        IRgenFcmpImmRight(b,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
        src=top_reg;
        IRgenZextI1toI32(b,src,++top_reg);
    }
}
void ConstExp::codeIR() { addexp->codeIR(); }
//√————————————————————完成————————————
void Lval::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    int lval_reg=irgen_table.symbol_table.lookup(name);
    Operand lval_ptr;
    bool formal_array_flag=false;
    VarAttribute val;
    if(lval_reg!=-1){//局部变量
        lval_ptr=GetNewRegOperand(lval_reg);
        if(irgen_table.formal_array_map[now_func].find(lval_reg)!=irgen_table.formal_array_map[now_func].end()){
            formal_array_flag=true;
            val=irgen_table.formal_array_map[now_func].find(lval_reg)->second;
        }
    }else{//全局变量
        lval_ptr=GetNewGlobalOperand(name->get_string());
    }
    ptr=lval_ptr;//变量的话就是存储位置，数组的话是数组首
    std::vector<Operand> index;
    if(!formal_array_flag){
        index.push_back(new ImmI32Operand(0));
    }
    if(dims!=nullptr){//有维度
        for(auto dim:*dims){
            dim->codeIR();
            int src=top_reg;
            Bin_ToInt(b,src,dim->GetNodeAttribute().T.type);
            index.push_back(GetNewRegOperand(top_reg));
        }
    }
    if(!left_flag){//这是个右值,要load
        if(!formal_array_flag){
            if(this->var.dims.size()!=0){//数组元素
                IRgenGetElementptrIndexI32(b,ToLT[var.type],++top_reg,lval_ptr,var.dims,index);
                int ptr=top_reg;
                if(GetNodeAttribute().T.type!=Type::PTR){
                    IRgenLoad(b,ToLT[var.type],++top_reg,GetNewRegOperand(ptr));
                }
            }else{//非数组
                IRgenLoad(b,ToLT[var.type],++top_reg,lval_ptr);
            }
        }else{//该左值是形参数组
            IRgenGetElementptrIndexI32(b,ToLT[var.type],++top_reg,lval_ptr,val.dims,index);
            int ptr=top_reg;
            if(GetNodeAttribute().T.type!=Type::PTR){
                IRgenLoad(b,ToLT[var.type],++top_reg,GetNewRegOperand(ptr));
            }
        }
    }else{
        //左值的时候，后期要store的，让左值ptr具有存储的值,需要调整数组
        if(!formal_array_flag){
            if(this->var.dims.size()!=0){//数组元素
                IRgenGetElementptrIndexI32(b,ToLT[var.type],++top_reg,lval_ptr,var.dims,index);
                ptr=GetNewRegOperand(top_reg);
            }
        }else{//该左值是形参数组
            IRgenGetElementptrIndexI32(b,ToLT[var.type],++top_reg,lval_ptr,val.dims,index);
            ptr=GetNewRegOperand(top_reg);
        }
    }
}
void FuncRParams::codeIR() {}

void Func_call::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(funcr_params==nullptr){
        if(attribute.T.type==Type::VOID){
            IRgenCallVoidNoArgs(b,BasicInstruction::LLVMType::VOID,name->get_string());
        }else{
            IRgenCallNoArgs(b,ToLT[attribute.T.type],++top_reg,name->get_string());
        }
    }else{
        std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args;
        std::vector<Expression> rparams;
        FuncDef funcdef=semant_table.FunctionTable[name];
        std::vector<FuncFParam> fparams=*(funcdef->formals);
        rparams=*((FuncRParams*)funcr_params)->params;
        
        int i=0;
        for(auto params:rparams){
            params->codeIR();
            if(params->GetNodeAttribute().T.type==Type::PTR){
                args.push_back({BasicInstruction::LLVMType::PTR,GetNewRegOperand(top_reg)});
            }else{//实参不是数组
                int src=top_reg;
                if(fparams[i]->GetNodeAttribute().T.type==Type::INT){
                    Bin_ToInt(b,src,params->GetNodeAttribute().T.type);
                }else if(fparams[i]->GetNodeAttribute().T.type==Type::FLOAT){
                    Bin_ToFloat(b,src,params->GetNodeAttribute().T.type);
                }
                args.push_back({ToLT[fparams[i]->GetNodeAttribute().T.type],GetNewRegOperand(top_reg)});
            }
            i++;
        }
        if(attribute.T.type==Type::VOID){
            IRgenCallVoid(b,BasicInstruction::LLVMType::VOID,args,name->get_string());
        }else{
            IRgenCall(b,ToLT[attribute.T.type],++top_reg,args,name->get_string());
        }
    }
}

//√————————————————————完成————————————
void UnaryExp_plus::codeIR() {
    unary_exp->codeIR();
    int reg1=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,unary_exp->GetNodeAttribute().T.type);
    }else if(GetNodeAttribute().T.type==Type::FLOAT){
        reg1=Bin_ToFloat(b,reg1,unary_exp->GetNodeAttribute().T.type);
    }
}
void UnaryExp_neg::codeIR() {
    unary_exp->codeIR();
    int reg1=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::INT){//目标INT
        reg1=Bin_ToInt(b,reg1,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticI32ImmLeft(b,BasicInstruction::LLVMIROpcode::SUB,0,reg1,++top_reg);
    }else if(GetNodeAttribute().T.type==Type::FLOAT){
        reg1=Bin_ToFloat(b,reg1,unary_exp->GetNodeAttribute().T.type);
        IRgenArithmeticF32ImmLeft(b,BasicInstruction::LLVMIROpcode::FSUB,0,reg1,++top_reg);
    }
}
void UnaryExp_not::codeIR() {
    unary_exp->codeIR();
    int reg1=top_reg;
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    if(GetNodeAttribute().T.type==Type::BOOL){//目标BOOL
        reg1=Bin_ToBool(b,reg1,unary_exp->GetNodeAttribute().T.type);
        IRgenZextI1toI32(b,reg1,++top_reg);
    }
}
void IntConst::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    //为一个整常数加一个寄存器。
    IRgenArithmeticI32ImmAll(b,BasicInstruction::LLVMIROpcode::ADD,val,0,++top_reg);
}
void FloatConst::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    //为一个浮点常数加一个寄存器。
    IRgenArithmeticF32ImmAll(b,BasicInstruction::LLVMIROpcode::FADD,val,0,++top_reg);
}
void StringConst::codeIR() {}
void PrimaryExp_branch::codeIR() { exp->codeIR(); }

void assign_stmt::codeIR() {
    //赋值语句，store指令
    lval->codeIR();
    exp->codeIR();//top_reg在这里弄
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    int exp_reg=top_reg;
    if(lval->var.type==Type::INT){
        Bin_ToInt(b,exp_reg,exp->GetNodeAttribute().T.type);
    }else if(lval->var.type==Type::FLOAT){
        Bin_ToFloat(b,exp_reg,exp->GetNodeAttribute().T.type);
    }
    IRgenStore(b, ToLT[lval->var.type],top_reg,((Lval*)lval)->ptr);
}
void expr_stmt::codeIR() { exp->codeIR(); }
void block_stmt::codeIR() {
    irgen_table.symbol_table.enter_scope();
    b->codeIR();
    irgen_table.symbol_table.exit_scope();
}
void ifelse_stmt::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock bbody = llvmIR.NewBlock(now_func, top_label);
    LLVMBlock belse = llvmIR.NewBlock(now_func, top_label);
    LLVMBlock bend = llvmIR.NewBlock(now_func, top_label);

    Cond->true_label=bbody->block_id;
    Cond->false_label=belse->block_id;
    Cond->codeIR();
    //条件判断
    int src=top_reg;
    LLVMBlock b1 = llvmIR.GetBlock(now_func, now_label);
    if(Cond->GetNodeAttribute().T.type==Type::FLOAT){ 
        IRgenFcmpImmRight(b1,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
    }else{
        IRgenIcmpImmRight(b1,BasicInstruction::IcmpCond::ne,src,0,++top_reg);
    }
    IRgenBrCond(b1,top_reg,bbody->block_id,belse->block_id);

    now_label=bbody->block_id;
    ifstmt->codeIR();
    LLVMBlock b2 = llvmIR.GetBlock(now_func, now_label);
    IRgenBRUnCond(b2,bend->block_id);

    now_label=belse->block_id;
    elsestmt->codeIR();
    LLVMBlock b3 = llvmIR.GetBlock(now_func, now_label);
    IRgenBRUnCond(b3,bend->block_id);

    now_label=bend->block_id;
}
void if_stmt::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock bbody = llvmIR.NewBlock(now_func, top_label);
    LLVMBlock bend = llvmIR.NewBlock(now_func, top_label);

    Cond->true_label=bbody->block_id;
    Cond->false_label=bend->block_id;
    Cond->codeIR();

    int src=top_reg;
    LLVMBlock b1 = llvmIR.GetBlock(now_func, now_label);
    if(Cond->GetNodeAttribute().T.type==Type::FLOAT){
        IRgenFcmpImmRight(b1,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
    }else{
        IRgenIcmpImmRight(b1,BasicInstruction::IcmpCond::ne,src,0,++top_reg);
    }
    IRgenBrCond(b1,top_reg,bbody->block_id,bend->block_id);

    now_label=bbody->block_id;
    ifstmt->codeIR();
    LLVMBlock b2 = llvmIR.GetBlock(now_func, now_label);
    IRgenBRUnCond(b2,bend->block_id);

    now_label=bend->block_id;
}
/*
while语句指令生成的伪代码：
    while的语法树节点为while(cond)stmt

    假设当前我们应该在B0基本块开始插入指令
    新建三个基本块Bcond，Bbody，Bend
    在B0基本块末尾插入一条无条件跳转指令，跳转到Bcond

    设置当前我们应该在Bcond开始插入指令
    cond->codeIR()    //在调用该函数前你可能需要设置真假值出口
    假设cond生成完后，我们应该在B1基本块继续插入指令，Bcond的结果为r0
    如果r0的类型不为bool，在B1末尾生成一条比较语句，比较r0是否为真。
    在B1末尾生成一条条件跳转语句，如果为真，跳转到Bbody，如果为假，跳转到Bend

    设置当前我们应该在Bbody开始插入指令
    stmt->codeIR()
    假设当stmt生成完后，我们应该在B2基本块继续插入指令
    在B2末尾生成一条无条件跳转语句，跳转到Bcond

    设置当前我们应该在Bend开始插入指令
*/
void while_stmt::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock bcond = llvmIR.NewBlock(now_func, top_label);
    LLVMBlock bbody = llvmIR.NewBlock(now_func, top_label);
    LLVMBlock bend = llvmIR.NewBlock(now_func, top_label);
    //暂存
    int cond_tmp=while_cond_label;
    int end_tmp=while_end_label;

    while_cond_label=bcond->block_id;
    while_end_label=bend->block_id;
    

    IRgenBRUnCond(b,bcond->block_id);
    now_label=bcond->block_id;
    Cond->true_label=bbody->block_id;
    Cond->false_label=bend->block_id;
    Cond->codeIR();//I32
    //I32->I1
    int src=top_reg;
    LLVMBlock b1 = llvmIR.GetBlock(now_func, now_label);
    if(Cond->GetNodeAttribute().T.type==Type::FLOAT){
        IRgenFcmpImmRight(b1,BasicInstruction::FcmpCond::ONE,src,0,++top_reg);
    }else{
        IRgenIcmpImmRight(b1,BasicInstruction::IcmpCond::ne,src,0,++top_reg);
    }
    IRgenBrCond(b1,top_reg,bbody->block_id,bend->block_id);

    now_label=bbody->block_id;
    body->codeIR();
    LLVMBlock b2 = llvmIR.GetBlock(now_func, now_label);
    IRgenBRUnCond(b2,bcond->block_id);

    now_label=bend->block_id;
    while_cond_label=cond_tmp;
    while_end_label=end_tmp;
}
void continue_stmt::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock new_block = llvmIR.NewBlock(now_func, top_label);
    //跳到起始点cond
    IRgenBRUnCond(b,while_cond_label);
    now_label=new_block->block_id;
}
void break_stmt::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    LLVMBlock new_block = llvmIR.NewBlock(now_func, top_label);
    //跳到结束点
    IRgenBRUnCond(b,while_end_label);
    now_label=new_block->block_id;
}
void return_stmt::codeIR() {
    return_exp->codeIR();//由其负责更新top寄存器
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    int src=top_reg;
    //需要隐式类型转换
    if(now_retty==BasicInstruction::LLVMType::I32){
        Bin_ToInt(b,src,return_exp->GetNodeAttribute().T.type);
    }else if(now_retty==BasicInstruction::LLVMType::FLOAT32){
        Bin_ToFloat(b,src,return_exp->GetNodeAttribute().T.type);
    }
    IRgenRetReg(b,now_retty,top_reg);
}
void return_stmt_void::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, now_label);
    IRgenRetVoid(b);
}
void ConstInitVal::codeIR() {}
void ConstInitVal_exp::codeIR() {
    exp->codeIR();
}
void VarInitVal::codeIR() {}
void VarInitVal_exp::codeIR() {
    exp->codeIR();
}
void VarDef_no_init::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, 0);//分配指针在当前函数最开始，IRgenAlloca函数插入的是0（指针数组的头部！）
    int reg=++top_reg;
    irgen_table.symbol_table.add_Symbol(name,reg);

    if(dims==nullptr){//非数组
        IRgenAlloca(b,ToLT[var.type],reg);
        b = llvmIR.GetBlock(now_func, now_label);//初始化
        if(var.type==Type::INT){
            IRgenArithmeticI32ImmAll(b,BasicInstruction::LLVMIROpcode::ADD,0,0,++top_reg);
        }else if(var.type==Type::FLOAT){
            IRgenArithmeticF32ImmAll(b,BasicInstruction::LLVMIROpcode::FADD,0,0,++top_reg);
        }
        IRgenStore(b,ToLT[var.type],top_reg,GetNewRegOperand(reg));
    }else{//数组
        IRgenAllocaArray(b,ToLT[var.type],reg,var.dims);
        b = llvmIR.GetBlock(now_func, now_label);//初始化
        int array_size=1;
        for(auto i:var.dims){
            array_size*=i;
        }
        // PTR：目标内存地址，指向要设置的内存块的起始位置。
        // i8 ：要设置的字节值，通常是一个 8 位的整数（i8）。
        // i32：要设置的字节数，指定从目标地址开始的内存区域的长度。
        // i1 ：一个布尔值，指示这个内存设置操作是否为易失性操作（volatile），即是否不能被优化或重排。
        std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args;
        args.push_back({BasicInstruction::LLVMType::PTR, GetNewRegOperand(reg)});
        args.push_back({BasicInstruction::LLVMType::I8, new ImmI32Operand(0)});
        args.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(array_size * sizeof(int))});
        args.push_back({BasicInstruction::LLVMType::I1, new ImmI32Operand(0)});
        IRgenCallVoid(b,BasicInstruction::LLVMType::VOID,args,std::string("llvm.memset.p0.i32"));
    } 
}
void VarDef::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, 0);//分配指针在当前函数最开始，IRgenAlloca函数插入的是0（指针数组的头部！）
    int reg=++top_reg;
    irgen_table.symbol_table.add_Symbol(name,reg);

    if(dims==nullptr){//非数组
        IRgenAlloca(b,ToLT[var.type],reg);
        b = llvmIR.GetBlock(now_func, now_label);//初始化
        InitVal init=GetInitVal();
        init->codeIR();
        int init_reg=top_reg;   
        if(var.type==Type::INT){
            Bin_ToInt(b,init_reg,init->GetExp()->GetNodeAttribute().T.type);
        }else if(var.type==Type::FLOAT){
            Bin_ToFloat(b,init_reg,init->GetExp()->GetNodeAttribute().T.type);
        }
        IRgenStore(b,ToLT[var.type],top_reg,GetNewRegOperand(reg));
    }else{//数组
        IRgenAllocaArray(b,ToLT[var.type],reg,var.dims);
        b = llvmIR.GetBlock(now_func, now_label);//初始化
        int array_size=1;
        for(auto i:var.dims){
            array_size*=i;
        }
        std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args;
        args.push_back({BasicInstruction::LLVMType::PTR, GetNewRegOperand(reg)});
        args.push_back({BasicInstruction::LLVMType::I8, new ImmI32Operand(0)});
        args.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(array_size * sizeof(int))});
        args.push_back({BasicInstruction::LLVMType::I1, new ImmI32Operand(0)});
        IRgenCallVoid(b,BasicInstruction::LLVMType::VOID,args,std::string("llvm.memset.p0.i32"));
        //设置初值
        InitVal init=GetInitVal();
        InitIntFloat(var.dims,init,0,reg,var.dims,attribute.T.type);
    }
}
void ConstDef::codeIR() {
    LLVMBlock b = llvmIR.GetBlock(now_func, 0);//分配指针在当前函数最开始，IRgenAlloca函数插入的是0（指针数组的头部！）
    int reg=++top_reg;
    irgen_table.symbol_table.add_Symbol(name,reg);

    if(dims==nullptr){//非数组
        IRgenAlloca(b,ToLT[var.type],reg);
        b = llvmIR.GetBlock(now_func, now_label);//初始化
        InitVal init=GetInitVal();
        init->codeIR();
        int init_reg=top_reg;   
        if(var.type==Type::INT){
            Bin_ToInt(b,init_reg,init->GetExp()->GetNodeAttribute().T.type);
        }else if(var.type==Type::FLOAT){
            Bin_ToFloat(b,init_reg,init->GetExp()->GetNodeAttribute().T.type);
        }
        IRgenStore(b,ToLT[var.type],top_reg,GetNewRegOperand(reg));
    }else{//数组
        IRgenAllocaArray(b,ToLT[var.type],reg,var.dims);
        b = llvmIR.GetBlock(now_func, now_label);//初始化
        int array_size=1;
        for(auto i:var.dims){
            array_size*=i;
        }
        // PTR：目标内存地址，指向要设置的内存块的起始位置。
        // i8 ：要设置的字节值，通常是一个 8 位的整数（i8）。
        // i32：要设置的字节数，指定从目标地址开始的内存区域的长度。
        // i1 ：一个布尔值，指示这个内存设置操作是否为易失性操作（volatile），即是否不能被优化或重排。
        std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args;
        args.push_back({BasicInstruction::LLVMType::PTR, GetNewRegOperand(reg)});
        args.push_back({BasicInstruction::LLVMType::I8, new ImmI32Operand(0)});
        args.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(array_size * sizeof(int))});
        args.push_back({BasicInstruction::LLVMType::I1, new ImmI32Operand(0)});
        IRgenCallVoid(b,BasicInstruction::LLVMType::VOID,args,std::string("llvm.memset.p0.i32"));
        //设置初值
        InitVal init=GetInitVal();
        InitIntFloat(var.dims,init,0,reg,var.dims,attribute.T.type);
    }
}
void VarDecl::codeIR() {
    for(auto var_def:*(GetDef())){
        var_def->codeIR();
    }
}
void ConstDecl::codeIR() {
    for(auto var_def:*(GetDef())){
        var_def->codeIR();
    }
}
void BlockItem_Decl::codeIR() { decl->codeIR(); }
void BlockItem_Stmt::codeIR() { stmt->codeIR(); }
void __Block::codeIR() {  
    auto item_vector = *item_list;
    for (auto item : item_vector) {
        item->codeIR();
    }
}
//√————————————————————————————————
void __FuncFParam::codeIR() {
}

void __FuncDef::codeIR() {
    irgen_table.symbol_table.enter_scope();
    now_retty=ToLT[return_type];
    FuncDefInstruction func_ins=new FunctionDefineInstruction(now_retty,name->get_string());
    llvmIR.NewFunction(func_ins);
    now_func=func_ins;//当前函数
    top_label=-1;
    LLVMBlock b=llvmIR.NewBlock(func_ins,top_label);
    now_label=top_label;
    
    //形参处理
    auto formal_vector = *formals;
    int start_reg=-1;
    top_reg=formal_vector.size()-1;
    
    for (auto formal : formal_vector) {
        start_reg++;
        if(formal->dims!=nullptr){
            func_ins->InsertFormal(BasicInstruction::LLVMType::PTR);
            irgen_table.symbol_table.add_Symbol(formal->name,start_reg);//数组形参，存原始位置
            VarAttribute val=formal->var;
            val.dims.erase(val.dims.begin());
            irgen_table.formal_array_map[now_func][start_reg]=val;
        }else{
            func_ins->InsertFormal(ToLT[formal->type_decl]);
            int reg=++top_reg;
            irgen_table.symbol_table.add_Symbol(formal->name,reg);
            IRgenAlloca(b,ToLT[formal->type_decl],reg);
            IRgenStore(b,ToLT[formal->type_decl],start_reg,GetNewRegOperand(reg));
        }
    }
    //块IR
    IRgenBRUnCond(b, 1);
    //块IR
    
    b = llvmIR.NewBlock(now_func, top_label);
    now_label = top_label;
    block->codeIR();
    //处理基本块返回
    for(auto idx:llvmIR.function_block_map[func_ins]){
        if(idx.second->Instruction_list.empty()){
            AddRet(idx.second,return_type);
        }else{
            Instruction ins=idx.second->Instruction_list.back();
            if(!(ins->GetOpcode()==BasicInstruction::LLVMIROpcode::RET||ins->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_COND||ins->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_UNCOND)){
                AddRet(idx.second,return_type);
            }
        }
    }
    llvmIR.func_max_map[func_ins]=std::make_pair(top_reg,top_label);
    irgen_table.symbol_table.exit_scope();
}
//√——————————————————————————————————————
void CompUnit_Decl::codeIR() {
    //全局声明
    for(auto var_def:*(decl->GetDef())){
        Instruction global_ins;
        VarAttribute val=var_def->var;
        if(val.dims.size()!=0){//数组
            global_ins=new GlobalVarDefineInstruction(var_def->GetName()->get_string(),ToLT[val.type],val);
        }else if(var_def->GetInitVal()==nullptr){
            global_ins=new GlobalVarDefineInstruction(var_def->GetName()->get_string(),ToLT[val.type],nullptr);
        }else if(val.type==Type::INT){
            global_ins=new GlobalVarDefineInstruction(var_def->GetName()->get_string(),ToLT[val.type],new ImmI32Operand(val.IntInitVals[0]));
        }else if(val.type==Type::FLOAT){
            global_ins=new GlobalVarDefineInstruction(var_def->GetName()->get_string(),ToLT[val.type],new ImmF32Operand(val.FloatInitVals[0]));
        }
        llvmIR.global_def.push_back(global_ins);
    }
}
void CompUnit_FuncDef::codeIR() { func_def->codeIR(); }
void AddLibFunctionDeclare() {
    FunctionDeclareInstruction *getint = new FunctionDeclareInstruction(BasicInstruction::I32, "getint");
    llvmIR.function_declare.push_back(getint);

    FunctionDeclareInstruction *getchar = new FunctionDeclareInstruction(BasicInstruction::I32, "getch");
    llvmIR.function_declare.push_back(getchar);

    FunctionDeclareInstruction *getfloat = new FunctionDeclareInstruction(BasicInstruction::FLOAT32, "getfloat");
    llvmIR.function_declare.push_back(getfloat);

    FunctionDeclareInstruction *getarray = new FunctionDeclareInstruction(BasicInstruction::I32, "getarray");
    getarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(getarray);

    FunctionDeclareInstruction *getfloatarray = new FunctionDeclareInstruction(BasicInstruction::I32, "getfarray");
    getfloatarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(getfloatarray);

    FunctionDeclareInstruction *putint = new FunctionDeclareInstruction(BasicInstruction::VOID, "putint");
    putint->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(putint);

    FunctionDeclareInstruction *putch = new FunctionDeclareInstruction(BasicInstruction::VOID, "putch");
    putch->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(putch);

    FunctionDeclareInstruction *putfloat = new FunctionDeclareInstruction(BasicInstruction::VOID, "putfloat");
    putfloat->InsertFormal(BasicInstruction::FLOAT32);
    llvmIR.function_declare.push_back(putfloat);

    FunctionDeclareInstruction *putarray = new FunctionDeclareInstruction(BasicInstruction::VOID, "putarray");
    putarray->InsertFormal(BasicInstruction::I32);
    putarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(putarray);

    FunctionDeclareInstruction *putfarray = new FunctionDeclareInstruction(BasicInstruction::VOID, "putfarray");
    putfarray->InsertFormal(BasicInstruction::I32);
    putfarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(putfarray);

    FunctionDeclareInstruction *starttime = new FunctionDeclareInstruction(BasicInstruction::VOID, "_sysy_starttime");
    starttime->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(starttime);

    FunctionDeclareInstruction *stoptime = new FunctionDeclareInstruction(BasicInstruction::VOID, "_sysy_stoptime");
    stoptime->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(stoptime);

    // 一些llvm自带的函数，也许会为你的优化提供帮助
    FunctionDeclareInstruction *llvm_memset =
    new FunctionDeclareInstruction(BasicInstruction::VOID, "llvm.memset.p0.i32");
    llvm_memset->InsertFormal(BasicInstruction::PTR);
    llvm_memset->InsertFormal(BasicInstruction::I8);
    llvm_memset->InsertFormal(BasicInstruction::I32);
    llvm_memset->InsertFormal(BasicInstruction::I1);
    llvmIR.function_declare.push_back(llvm_memset);

    FunctionDeclareInstruction *llvm_umax = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.umax.i32");
    llvm_umax->InsertFormal(BasicInstruction::I32);
    llvm_umax->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_umax);

    FunctionDeclareInstruction *llvm_umin = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.umin.i32");
    llvm_umin->InsertFormal(BasicInstruction::I32);
    llvm_umin->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_umin);

    FunctionDeclareInstruction *llvm_smax = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.smax.i32");
    llvm_smax->InsertFormal(BasicInstruction::I32);
    llvm_smax->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_smax);

    FunctionDeclareInstruction *llvm_smin = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.smin.i32");
    llvm_smin->InsertFormal(BasicInstruction::I32);
    llvm_smin->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_smin);
}
