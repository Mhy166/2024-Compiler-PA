#include "mem2reg.h"
#include <tuple>
#include <algorithm>
#include <Instruction.h>
#include <IRgen.h>

//extern int top_reg;
static std::set<Instruction> EraseInsSet;     //删除指令集
static std::map<int, int> mem2reg_map;    //替换map <old regno, new regno>
static std::set<int> CommonAllocaSet;      // 一般情况的的alloca
static std::map<PhiInstruction *, int> phi_map;     


// 检查该条alloca指令是否可以被mem2reg
// eg. 数组不可以mem2reg
// eg. 如果该指针直接被使用不可以mem2reg(在SysY一般不可能发生,SysY不支持指针语法)
bool Mem2RegPass::IsPromotable(CFG *C, Instruction AllocaInst) {
    if(AllocaInst->GetOpcode()==BasicInstruction::LLVMIROpcode::ALLOCA&& ((AllocaInstruction*)AllocaInst)->GetDims().empty()){
        return true;
    }
    return false;
}

/*
    int a1 = 5,a2 = 3,a3 = 11,b = 4
    return b // a1,a2,a3 is useless
-----------------------------------------------
pseudo IR is:
    %r0 = alloca i32 ;a1
    %r1 = alloca i32 ;a2
    %r2 = alloca i32 ;a3
    %r3 = alloca i32 ;b
    store 5 -> %r0 ;a1 = 5
    store 3 -> %r1 ;a2 = 3
    store 11 -> %r2 ;a3 = 11
    store 4 -> %r3 ;b = 4
    %r4 = load i32 %r3
    ret i32 %r4
--------------------------------------------------
%r0,%r1,%r2只有store, 但没有load,所以可以删去
优化后的IR(pseudo)为:
    %r3 = alloca i32
    store 4 -> %r3
    %r4 - load i32 %r3
    ret i32 %r4
*/

// vset is the set of alloca regno that only store but not load
// 该函数对你的时间复杂度有一定要求, 你需要保证你的时间复杂度小于等于O(nlognlogn), n为该函数的指令数
// 提示:deque直接在中间删除是O(n)的, 可以先标记要删除的指令, 最后想一个快速的方法统一删除
void Mem2RegPass::Mem2RegNoUseAlloca(CFG *C, std::set<int> &vset) {  
    for(auto block_item:*(C->block_map)){
        //std::deque<Instruction> new_deque;
        LLVMBlock block=block_item.second;
        for(auto ins:block->Instruction_list){
            if(ins->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
                if(((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetOperandType()==BasicOperand::operand_type::REG){
                    if(vset.find(((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo())!=vset.end()){
                        EraseInsSet.insert(ins);
                        continue;
                    }
                }
            }
        }      
    }
}

/*
    int b = getint();
    b = b + 10
    return b // def and use of b are in same block
-----------------------------------------------
pseudo IR is:
    %r0 = alloca i32 ;b
    %r1 = call getint()
    store %r1 -> %r0
    %r2 = load i32 %r0
    %r3 = %r2 + 10
    store %r3 -> %r0
    %r4 = load i32 %r0
    ret i32 %r4
--------------------------------------------------
%r0的所有load和store都在同一个基本块内
优化后的IR(pseudo)为:
    %r1 = call getint()
    %r3 = %r1 + 10
    ret %r3

对于每一个load，我们只需要找到最近的store,然后用store的值替换之后load的结果即可
*/

// vset is the set of alloca regno that load and store are all in the BB block_id
// 该函数对你的时间复杂度有一定要求，你需要保证你的时间复杂度小于等于O(nlognlogn), n为该函数的指令数
void Mem2RegPass::Mem2RegUseDefInSameBlock(CFG *C, std::set<int> &vset, int block_id) {
    std::map<int, int> curr_reg_map;
    
    LLVMBlock block=(*(C->block_map))[block_id];
    //new_deque.clear();
    for(auto ins:block->Instruction_list){
        if(ins->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
            int value=((RegOperand*)((StoreInstruction*)ins)->GetValue())->GetRegNo();
            if(((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetOperandType()==BasicOperand::operand_type::REG){
                int ptr=((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo();
                if(vset.find(ptr)!=vset.end()){
                    curr_reg_map[ptr] = ((RegOperand *)(((StoreInstruction *)ins)->GetValue()))->GetRegNo();
                    EraseInsSet.insert(ins);
                    //store_map[ptr]=value;
                    continue;
                }
            }
        }
        if(ins->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
            int res=((RegOperand*)((LoadInstruction*)ins)->GetResult())->GetRegNo();
            if(((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetOperandType()==BasicOperand::operand_type::REG){
                int ptr=((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo();
                if(vset.find(ptr)!=vset.end()){
                    //mem2reg_map[res]=store_map[ptr];
                    mem2reg_map[((RegOperand*)((LoadInstruction *)ins)->GetResult())->GetRegNo()] = curr_reg_map[ptr];
                    EraseInsSet.insert(ins);
                    continue;
                }
            }
        }
        //ins->Reg_Rename(mem2reg_map);
        //new_deque.push_back(ins);
    }
    //block->Instruction_list=new_deque;
}

// vset is the set of alloca regno that one store dominators all load instructions
// 该函数对你的时间复杂度有一定要求，你需要保证你的时间复杂度小于等于O(nlognlogn)
void Mem2RegPass::Mem2RegOneDefDomAllUses(CFG *C, std::set<int> &vset) {
    std::map<int, int> v_result_map;
    for (auto block_item : *C->block_map) {
        LLVMBlock block=block_item.second;
        for (auto ins : block->Instruction_list) {
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
                auto StoreI = (StoreInstruction *)ins;
                if (((StoreInstruction *)ins)->GetPointer()->GetOperandType() == BasicOperand::REG) {
                    int ptr = ((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo();
                    if (vset.find(ptr) != vset.end()) {
                        v_result_map[ptr] = ((RegOperand *)(StoreI->GetValue()))->GetRegNo();
                        EraseInsSet.insert(ins);
                    }
                }
            }
        }
    }
    for (auto block_item : *C->block_map) {
        LLVMBlock block=block_item.second;
        for (auto ins : block->Instruction_list) {
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD) {
                auto LoadI = (LoadInstruction *)ins;
                if (LoadI->GetPointer()->GetOperandType() == BasicOperand::REG) {
                    int ptr = ((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo();
                    if (vset.find(ptr) != vset.end()) {
                        mem2reg_map[((RegOperand*)((LoadInstruction *)ins)->GetResult())->GetRegNo()] = v_result_map[ptr];
                        EraseInsSet.insert(ins);
                    }
                }
            }
        }
    }
}


void Mem2RegPass::InsertPhi(CFG *C) {
    DominatorTree *domTree = domtrees->GetDomTree(C);

    for (auto &[id, block] : *C->block_map) {
        auto tmp_list = block->Instruction_list;
        block->Instruction_list.clear();
        for (auto ins : tmp_list) {
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
                auto StoreI = (StoreInstruction *)ins;
                auto val = ((StoreInstruction *)ins)->GetValue();
                if (val->GetOperandType() == BasicOperand::IMMI32) {
                    auto ArithI =new ArithmeticInstruction(AllocaInstruction::ADD, AllocaInstruction::I32, val, new ImmI32Operand(0), GetNewRegOperand(++C->top_reg));
                    block->Instruction_list.push_back(ArithI);
                    ((StoreInstruction *)ins)->SetValue(GetNewRegOperand(C->top_reg));
                } else if (val->GetOperandType() == BasicOperand::IMMF32) {
                    auto ArithI =new ArithmeticInstruction(AllocaInstruction::FADD, AllocaInstruction::FLOAT32, val, new ImmF32Operand(0), GetNewRegOperand(++C->top_reg));
                    block->Instruction_list.push_back(ArithI);
                    ((StoreInstruction *)ins)->SetValue(GetNewRegOperand(C->top_reg));
                }
            }
            block->Instruction_list.push_back(ins);
        }
    }

    //记录storge寄存器（def） 寄存器storge次数（def_num） load寄存器（uses）
    //map<int, std::set<int>> defs, uses   <regno, set<block_id>>
    //map<int, int> def_num    <alloca regno,storge数目>
    std::map<int, std::set<int>> defs, uses;
    std::map<int, int> def_num;
    for (auto [id, block] : *C->block_map) {
        for (auto ins : block->Instruction_list) {
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
                if (((StoreInstruction *)ins)->GetPointer()->GetOperandType() != BasicOperand::GLOBAL) {
                    defs[((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo()].insert(id);               
                    def_num[((RegOperand*)((StoreInstruction*)ins)->GetPointer())->GetRegNo()]++;
                    continue;
                }
            } else if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD) {
                if (((LoadInstruction *)ins)->GetPointer()->GetOperandType() != BasicOperand::GLOBAL) {
                    uses[((RegOperand*)((LoadInstruction*)ins)->GetPointer())->GetRegNo()].insert(id);
                    continue;
                }
            }
        }
    }



    LLVMBlock entry_Block = (*C->block_map)[0];
    std::set<int> no_use_vset, one_dom_vset;
    std::map<int, std::set<int>> same_block_vset_map;

    for (auto ins : entry_Block->Instruction_list) {
        // 不是可处理的 ALLOCA 
        if (!IsPromotable(C, ins)) {
            continue;  
        }   
        
        int result_reg = ((RegOperand *)((AllocaInstruction *)ins)->GetResult())->GetRegNo();
        AllocaInstruction::LLVMType type = ((AllocaInstruction *)ins)->GetDataType();

        auto alloca_defs = defs[result_reg];
        auto alloca_uses = uses[result_reg];
        if (alloca_uses.size() == 0) {    // NoUse
            EraseInsSet.insert(ins);
            no_use_vset.insert(result_reg);
            continue;
        }

        if (alloca_defs.size() == 1) {
            int block_id = *(alloca_defs.begin());
            if (alloca_uses.size() == 1 && *(alloca_uses.begin()) == block_id) {    // UseDefInSameBlock
                EraseInsSet.insert(ins);
                same_block_vset_map[block_id].insert(result_reg);
                continue;
            }
        }

        if (def_num[result_reg] == 1) {    // 定义一次
            int block_id = *(alloca_defs.begin());
            int dom_flag = 1;
            for (auto load_BBid : alloca_uses) {
                if (domTree->IsDominate(block_id, load_BBid) == false) {
                    dom_flag = 0;
                    break;
                }
            }
            if (dom_flag) {    // OneDefDomAllUses
                EraseInsSet.insert(ins);
                one_dom_vset.insert(result_reg);
                continue;
            }
        }

        //一般情况
        CommonAllocaSet.insert(result_reg);
        EraseInsSet.insert(ins);
        std::set<int> F{};            // 添加phi的块集合
        std::set<int> W = defs[result_reg];    // 定义regno的块集合

        while (!W.empty()) {
            int BB_X = *W.begin();
            W.erase(BB_X);
            for (auto BB_Y : domTree->GetDF(BB_X)) {
                if (F.find(BB_Y) == F.end()) {
                    PhiInstruction *PhiI = new PhiInstruction(type, GetNewRegOperand(++C->top_reg));
                    (*C->block_map)[BB_Y]->InsertInstruction(0, PhiI);
                    phi_map[PhiI] = result_reg;
                    F.insert(BB_Y);
                    if (defs[result_reg].find(BB_Y) == defs[result_reg].end()) {
                        W.insert(BB_Y);
                    }
                }
            }
        }
    }
    Mem2RegNoUseAlloca(C, no_use_vset);
    Mem2RegOneDefDomAllUses(C, one_dom_vset);
    for (auto [id, vset] : same_block_vset_map) {
        Mem2RegUseDefInSameBlock(C, vset, id);
    }
}


//reference！ 变量映射重命名部分有参考助教原框架代码！
int ls_in_allocas(std::set<int> &S, Instruction ins) {
    if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD) {
        if (((LoadInstruction *)ins)->GetPointer()->GetOperandType() == BasicOperand::REG) {
            int ptr = ((RegOperand *)(((LoadInstruction *)ins)->GetPointer()))->GetRegNo();
            if (S.find(ptr) != S.end()) {
                return ptr;
            }
        }
        return -1;
    }
    if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
        if (((StoreInstruction *)ins)->GetPointer()->GetOperandType() == BasicOperand::REG) {
            int ptr = ((RegOperand *)(((StoreInstruction *)ins)->GetPointer()))->GetRegNo();
            if (S.find(ptr) != S.end()) {
                return ptr;
            }
        }
        return -1;
    }
    return -1;
}

void Mem2RegPass::VarRename(CFG *C) {
    std::map<int, std::map<int, int>> WorkList;    //< BB, <alloca_reg,val_reg> >
    WorkList.insert({0, std::map<int, int>{}});
    std::vector<int> BBvis;
    BBvis.resize(C->top_label+1);
    while (!WorkList.empty()) {
        int BB = (*WorkList.begin()).first;
        auto IncomingVals = (*WorkList.begin()).second;
        WorkList.erase(BB);
        if (BBvis[BB]) {
            continue;
        }
        BBvis[BB] = 1;
        for (auto &ins : (*C->block_map)[BB]->Instruction_list) {
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD) {
                int reg = ls_in_allocas(CommonAllocaSet, ins);
                if (reg >= 0) {   // 如果当前指令是 load，找到对应的 alloca v，将用到 load 结果的地方都替换成IncomingVals[v]
                    EraseInsSet.insert((LoadInstruction *)ins);
                    mem2reg_map[((RegOperand*)((LoadInstruction *)ins)->GetResult())->GetRegNo()] = IncomingVals[reg];
                }
            }
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
                int reg = ls_in_allocas(CommonAllocaSet, ins);
                if (reg >= 0) {    // 如果当前指令是 store，找到对应的 alloca v，更新IncomingVals[v] = val,并删除store
                    EraseInsSet.insert((StoreInstruction *)ins);
                    IncomingVals[reg] = ((RegOperand *)(((StoreInstruction *)ins)->GetValue()))->GetRegNo();
                }
            }
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                if (EraseInsSet.find((PhiInstruction *)ins) != EraseInsSet.end()) {
                    continue;
                }
                auto it = phi_map.find((PhiInstruction *)ins);
                if (it != phi_map.end()) {    // 更新IncomingVals[v] = val
                    IncomingVals[it->second] = ((PhiInstruction *)ins)->GetResultRegNo();
                }
            }
        }
        for (auto succ : C->G[BB]) {
            int BBv = succ->block_id;
            WorkList.insert({BBv, IncomingVals});
            for (auto ins : (*C->block_map)[BBv]->Instruction_list) {
                if (ins->GetOpcode() != BasicInstruction::LLVMIROpcode::PHI) {
                    break;
                }
                auto PhiI = (PhiInstruction *)ins;// 找到 phi 对应的 alloca
                auto it = phi_map.find(PhiI);
                if (it != phi_map.end()) {
                    int v = it->second;
                    if (IncomingVals.find(v) == IncomingVals.end()) {
                        EraseInsSet.insert(PhiI);
                        continue;
                    }
                    // 为 phi 添加前驱块到当前块的边
                    PhiI->InsertPhi(GetNewRegOperand(IncomingVals[v]), GetNewLabelOperand(BB));
                }
            }
        }
    }

    for (auto [id, block] : *C->block_map) {
        for (auto ins : block->Instruction_list) {
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD && ((LoadInstruction *)ins)->GetPointer()->GetOperandType() == BasicOperand::REG) {
                int result = ((RegOperand*)((LoadInstruction *)ins)->GetResult())->GetRegNo();
                if (mem2reg_map.find(result) != mem2reg_map.end()) {
                    int result2 = mem2reg_map[result];
                    while (mem2reg_map.find(result2) != mem2reg_map.end()) {
                        mem2reg_map[result] = mem2reg_map[result2];
                        result2 = mem2reg_map[result];
                    }
                }
            }
        }
    }

    for (auto [id, block] : *C->block_map) {
        auto tmp_Instruction_list = block->Instruction_list;
        block->Instruction_list.clear();
        for (auto ins : tmp_Instruction_list) {
            if (EraseInsSet.find(ins) != EraseInsSet.end()) {
                continue;
            }
            block->InsertInstruction(1, ins);
        }
    }

    for (auto B1 : *C->block_map) {
        for (auto ins : B1.second->Instruction_list) {
            ins->Reg_Rename(mem2reg_map);
        }
    }

    EraseInsSet.clear();
    mem2reg_map.clear();
    CommonAllocaSet.clear();
    phi_map.clear();
}

void Mem2RegPass::Mem2Reg(CFG *C) {
    InsertPhi(C);
    VarRename(C);
}

void Mem2RegPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        Mem2Reg(cfg);
    }
}