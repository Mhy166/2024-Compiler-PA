#include "dce.h"
#include <algorithm>

void DCEPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        DCE(cfg);
        ADCE(cfg);
    }
}
void DCEPass::DCE(CFG *C){
    std::map<int, Instruction> Result2Ins;
    std::set<int> DCEWorkSet;
    std::map<int, int> Reg2Use;
    std::set<Instruction> EraseSet;

    for (auto block_item : *C->block_map) {
        auto block = block_item.second;
        for (auto ins : block->Instruction_list) {
            int v = ins->GetResultRegNo();
            //获取每一条指令的结果寄存器号
            if (v != -1) {   
                DCEWorkSet.insert(v);
                Result2Ins[v] = ins;
            }
            //给每个用到的寄存器的使用次数+1
            for (auto op : ins->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    Reg2Use[((RegOperand *)op)->GetRegNo()]++;
                }
            }
        }
    }
    //迭代检查是否用到！
    while (!DCEWorkSet.empty()) {
        int reg = *DCEWorkSet.begin();
        DCEWorkSet.erase(reg);
        if (!Reg2Use[reg]) {//没有用到
            auto result_ins = Result2Ins[reg];
            //Call指令我们简化了，选择不处理，否则可能会引发输入错乱的问题！
            if (result_ins == nullptr||result_ins->GetOpcode() == BasicInstruction::LLVMIROpcode::CALL) {
                continue;
            }   
            EraseSet.insert(result_ins);
            for (auto op : result_ins->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    int x = ((RegOperand *)op)->GetRegNo();
                    Reg2Use[x]--;
                    DCEWorkSet.insert(x);
                }
            }
        }
    }
    //删除
    for (auto block_item : *C->block_map) {
        auto block=block_item.second;
        auto tmp_list = block->Instruction_list;
        block->Instruction_list.clear();
        for (auto I : tmp_list) {
            if (EraseSet.find(I) != EraseSet.end()) {
                continue;
            }
            block->InsertInstruction(1, I);
        }
    }
}


void DCEPass::ADCE(CFG *C){
    //逆序支配树
    DominatorTree *PostDomTree = postdomtrees->GetPostDomTree(C);
    //reference！ ADCE部分代码有参考助教原框架代码！但逆支配关系是自己编写的

    std::deque<Instruction> worklist;//待处理的指令队列
    std::map<int, Instruction> defmap;//保存每个寄存器的定义指令
    std::set<Instruction> liveInsSet;//存储活跃指令集合
    std::set<int> liveBBset;//存储包含活跃指令的基本块

    std::vector<std::vector<LLVMBlock>> CDG;
    std::vector<std::vector<LLVMBlock>> CDG_precursor;
    std::vector<int> rd;
    auto G = C->G;
    auto invG = C->invG;
    
    auto blockmap = (*C->block_map);
    CDG.resize(C->top_label + 2);
    rd.resize(C->top_label + 1, 0);
    CDG_precursor.resize(C->top_label + 1);
    for (int i = 0; i <= C->top_label; ++i) {
        auto domFrontier = PostDomTree->GetDF(i);
        for (auto vbbid : domFrontier) {
            CDG[vbbid].push_back(blockmap[i]);
            if (vbbid != i) {
                rd[i]++;
            }
            CDG_precursor[blockmap[i]->block_id].push_back(blockmap[vbbid]);
        }
    }
    for (int i = 0; i <= C->top_label; ++i) {
        if (!rd[i]&&blockmap.find(i)!=blockmap.end()) {
            CDG[C->top_label + 1].push_back(blockmap[i]);
        }
    }

    auto PostDomTreeidom = PostDomTree->idom;
    
    for (auto [id, bb] : blockmap) {
        for (auto I : bb->Instruction_list) {
            I->BlockID=id;
            if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE || 
            I->GetOpcode() == BasicInstruction::LLVMIROpcode::CALL || 
            I->GetOpcode() == BasicInstruction::LLVMIROpcode::RET) {
                worklist.push_back(I);
            }
            if (I->GetResultReg() != nullptr) {
                defmap[I->GetResultRegNo()] = I;
            }
        }
    }

    while (!worklist.empty()) {
        auto I = worklist.front();
        worklist.pop_front();
        if (liveInsSet.find(I) != liveInsSet.end()) {
            continue;
        }
        liveInsSet.insert(I);
        auto parBBno = I->BlockID;
        auto parBB = blockmap[I->BlockID];
        liveBBset.insert(parBBno);
        if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
            auto PhiI = (PhiInstruction *)I;
            for (auto [Labelop, Regop] : PhiI->GetPhiList()) {
                auto Label = (LabelOperand *)Labelop;
                auto Labelno = Label->GetLabelNo();

                auto blockmap = (*C->block_map);
                auto bb = blockmap[Labelno];
                auto terminalI= bb->Instruction_list.back();

                if (liveInsSet.find(terminalI) == liveInsSet.end()) {
                    worklist.push_front(terminalI);
                    liveBBset.insert(Labelno);
                }
            }
        }

        if (parBBno != -1) {
            for (auto CDG_pre : CDG_precursor[parBBno]) {
                auto CDG_preno = CDG_pre->block_id;
                auto blockmap = (*C->block_map);
                auto bb = blockmap[CDG_preno];
                auto terminalI= bb->Instruction_list.back();
                if (liveInsSet.find(terminalI) == liveInsSet.end()) {
                    worklist.push_front(terminalI);
                }
            }
        }

        for (auto op : I->GetNonResultOperands()) {
            if (op->GetOperandType() == BasicOperand::REG) {
                auto Regop = (RegOperand *)op;
                auto Regopno = Regop->GetRegNo();
                if (defmap.find(Regopno) == defmap.end()) {
                    continue;
                }
                auto DefI = defmap[Regopno];
                if (liveInsSet.find(DefI) == liveInsSet.end()) {
                    worklist.push_front(DefI);
                }
            }
        }
    }

    for (auto [id, bb] : *C->block_map) {
        auto blockmap = (*C->block_map);
        auto terminalI= bb->Instruction_list.back();
        auto tmp_Instruction_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto I : tmp_Instruction_list) {
            if (liveInsSet.find(I) == liveInsSet.end()) {
                if (terminalI == I) {
                    auto livebbid = PostDomTreeidom[id]->block_id;
                    while (liveBBset.find(livebbid) == liveBBset.end()) {
                        livebbid = PostDomTreeidom[id]->block_id;
                    }
                    I = new BrUncondInstruction(GetNewLabelOperand(livebbid));
                } else {
                    continue;
                }
            }
            bb->InsertInstruction(1, I);
        }
    }
    
    defmap.clear();
    liveInsSet.clear();
    liveBBset.clear();
    
}