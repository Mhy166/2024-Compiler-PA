#include "simplify_cfg.h"
#include <algorithm>

void SimplifyCFGPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
    }
}

// 删除从函数入口开始到达不了的基本块和指令
// 不需要考虑控制流为常量的情况，你只需要从函数入口基本块(0号基本块)开始dfs，将没有被访问到的基本块和指令删去即可
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) {
    std::set<int> reachable_blocks;
    //消除指令
    for(auto &[block_id,block]:*(C->block_map)){
        int deque_size=block->Instruction_list.size();
        int i=0;
        for(auto &ins:block->Instruction_list){
            i++;
            if(ins->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_COND||
            ins->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_UNCOND||
            ins->GetOpcode()==BasicInstruction::LLVMIROpcode::RET)
            {
                break;
            }
        }
        int delete_size=deque_size-i;
        for(int j=1;j<=delete_size;j++){
            block->Instruction_list.pop_back();
        }
    }
    //在删除不可达指令后，重新构建一下。
    C->BuildCFG();
    //消除块
    std::stack<int> dfs_block;
    dfs_block.push(0);
    while(!dfs_block.empty()){
        int curr_block_id=dfs_block.top();
        reachable_blocks.insert(curr_block_id);
        dfs_block.pop();
        for(auto succ:C->G[curr_block_id]){
            if(reachable_blocks.find(succ->block_id)==reachable_blocks.end()){
                dfs_block.push(succ->block_id);
            }
        }
    }
    std::set<int> eliminate_blocks;
    std::set<int> all_blocks;
    for (auto block_item : *(C->block_map)) {
        all_blocks.insert(block_item.first);  
    }
    std::set_difference(all_blocks.begin(),all_blocks.end(),reachable_blocks.begin(),reachable_blocks.end(),std::inserter(eliminate_blocks,eliminate_blocks.end()));
    
    for (auto block_id : eliminate_blocks) {
        C->block_map->erase(block_id);  // 删除基本块
    }
    
    
    // std::map<int, int> old_to_new_id;//id映射表
    // int new_id = 0;
    // std::set<int> new_block_ids;

    // for (auto &block_item : *(C->block_map)) {
    //     old_to_new_id[block_item.first] = new_id;
    //     new_block_ids.insert(new_id);
    //     new_id++;
    // }

    // // 分配连续的 block_id
    // std::map<int, LLVMBlock> new_map;
    // for (auto &block_item : *(C->block_map)) {
    //     LLVMBlock bb = block_item.second;
    //     new_map[old_to_new_id[block_item.first]] = bb;
    // }

    // C->block_map->clear();
    // for (auto &item : new_map) {
    //     C->block_map->insert(item);
    // }

    // // 更新跳转标签
    // for (auto &block_item : *(C->block_map)) {
    //     LLVMBlock bb = block_item.second;
    //     for (auto &ins : bb->Instruction_list) {
    //         if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_COND) {
    //             BrCondInstruction *br_cond = (BrCondInstruction *)ins;
    //             int true_block_id = ((LabelOperand *)br_cond->GetTrueLabel())->GetLabelNo();
    //             int false_block_id = ((LabelOperand *)br_cond->GetFalseLabel())->GetLabelNo();

    //             if (reachable_blocks.find(true_block_id) == reachable_blocks.end()) {
    //                 int new_true_block_id = old_to_new_id[true_block_id];
    //                 ((LabelOperand *)br_cond->GetTrueLabel())->SetLabelNo(new_true_block_id);
    //             }
    //             if (reachable_blocks.find(false_block_id) == reachable_blocks.end()) {
    //                 int new_false_block_id = old_to_new_id[false_block_id];
    //                 ((LabelOperand *)br_cond->GetFalseLabel())->SetLabelNo(new_false_block_id);
    //             }
    //         }
    //         else if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) {
    //             BrUncondInstruction *br_uncond = (BrUncondInstruction *)ins;
    //             int dest_block_id = ((LabelOperand *)br_uncond->GetDestLabel())->GetLabelNo();

    //             if (reachable_blocks.find(dest_block_id) == reachable_blocks.end()) {
    //                 int new_dest_block_id = old_to_new_id[dest_block_id];
    //                 ((LabelOperand *)br_uncond->GetDestLabel())->SetLabelNo(new_dest_block_id);
    //             }
    //         }
    //     }
    // }


    // 更新 block_map 和重新构建 CFG
    C->BuildCFG();
}