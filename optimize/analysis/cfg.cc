#include "../../include/Instruction.h"
#include "../../include/ir.h"
#include <assert.h>

void LLVMIR::CFGInit() {
    for (auto &[defI, bb_map] : function_block_map) {//对每个函数，都产生一个CFG
        CFG *cfg = new CFG();
        cfg->block_map = &bb_map;
        cfg->function_def = defI;
        cfg->top_reg = func_max_map[defI].first;//IR传给CFG的
        cfg->top_label=func_max_map[defI].second;
        cfg->BuildCFG();
        llvm_cfg[defI] = cfg;
    }
}

void LLVMIR::BuildCFG() {
    for (auto [defI, cfg] : llvm_cfg) {
        cfg->BuildCFG();
    }
}

void CFG::BuildCFG() {
    G.clear();
    invG.clear();
    //可能删除块，为了对应映射，这里不能用block_map->size()!
    G.resize(top_label+1);
    invG.resize(top_label+1);
    //逆支配需要统一的ret块，我们再逆支配前先统一各个ret块后，再构建cfg这样保证ret_block可以逆向追踪到所有的块！
    for (auto block_item : *block_map) {
        LLVMBlock block = block_item.second;
        Instruction last_Ins = block->Instruction_list[block->Instruction_list.size() - 1];
        if (last_Ins->GetOpcode() == BasicInstruction::LLVMIROpcode::RET) 
        {
            ret_block = block;
        }
        else if (last_Ins->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) //无条件跳转
        {
            BrUncondInstruction *BrUn_Ins = (BrUncondInstruction *)last_Ins;
            int target_block_no = ((LabelOperand *)BrUn_Ins->GetDestLabel())->GetLabelNo();
            G[block->block_id].push_back((*block_map)[target_block_no]);
            invG[target_block_no].push_back(block);
        } 
        else if (last_Ins->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_COND)//条件跳转双分支
        {
            BrCondInstruction *Br_Ins = (BrCondInstruction *)last_Ins;
            int target_trueblock_no = ((LabelOperand *)Br_Ins->GetTrueLabel())->GetLabelNo();
            int target_falseblock_no = ((LabelOperand *)Br_Ins->GetFalseLabel())->GetLabelNo();
            G[block->block_id].push_back((*block_map)[target_trueblock_no]);
            G[block->block_id].push_back((*block_map)[target_falseblock_no]);
            invG[target_trueblock_no].push_back(block);
            invG[target_falseblock_no].push_back(block);
        } 
    }
}


std::vector<LLVMBlock> CFG::GetPredecessor(LLVMBlock B) { return invG[B->block_id]; }

std::vector<LLVMBlock> CFG::GetPredecessor(int bbid) { return invG[bbid]; }

std::vector<LLVMBlock> CFG::GetSuccessor(LLVMBlock B) { return G[B->block_id]; }

std::vector<LLVMBlock> CFG::GetSuccessor(int bbid) { return G[bbid]; }

LLVMBlock CFG::GetBlock(int block_id) { return (*block_map)[block_id]; }

LLVMBlock CFG::NewBlock() {
    LLVMBlock new_block = new BasicBlock(++top_label);
    (*block_map)[top_label] = new_block;
    return new_block;
}