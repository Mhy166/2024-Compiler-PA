#include "dominator_tree.h"
#include "../../include/ir.h"
#include <algorithm>
//构建支配关系前，先统一结尾ret块，否则多ret下逆向支配关系会有遗漏
void MakeFunctionOneExit(CFG *C) {
    std::queue<LLVMBlock> Ret_Block_queue;
    enum AllocaInstruction::LLVMType ret_type = AllocaInstruction::VOID;
    int ret_ins_cnt = 0;
    //计算返回块列表
    for (auto block_item : *C->block_map) {
        LLVMBlock block = block_item.second;
        Instruction last_Ins = block->Instruction_list[block->Instruction_list.size() - 1];
        if (last_Ins->GetOpcode() != AllocaInstruction::RET) {
            continue;
        }
        ret_ins_cnt++;
        ret_type = ((RetInstruction *)last_Ins)->GetType();
        Ret_Block_queue.push(block);
    }
    //如若没有返回块
    if (ret_ins_cnt == 0) {
        C->block_map->clear();
        C->top_label = C->top_reg= -1;
        LLVMBlock block = C->NewBlock();
        if (C->function_def->GetReturnType() == AllocaInstruction::VOID) {
            block->InsertInstruction(1, new RetInstruction(AllocaInstruction::VOID, nullptr));
        } else if (C->function_def->GetReturnType() == AllocaInstruction::I32) {
            block->InsertInstruction(1, new RetInstruction(AllocaInstruction::I32, new ImmI32Operand(0)));
        } else if (C->function_def->GetReturnType() == AllocaInstruction::FLOAT32) {
            block->InsertInstruction(1, new RetInstruction(AllocaInstruction::FLOAT32, new ImmF32Operand(0)));
        } 
        C->ret_block = block;
        C->BuildCFG();
        return;
    }
    else if (ret_ins_cnt == 1) {
        C->ret_block = Ret_Block_queue.front();
        return;
    }
    else{
        //多个块，需要统一
        LLVMBlock block = C->NewBlock();
        if (ret_type != AllocaInstruction::VOID) {//本函数返回类型不空！    
            auto ret_reg = GetNewRegOperand(++C->top_reg);//都往该返回值寄存器赋值！
            auto One_Ret_reg = GetNewRegOperand(++C->top_reg);           
            ((*(C->block_map))[0])->InsertInstruction(0, new AllocaInstruction(ret_type, ret_reg));//为这个多路的值分配寄存器
            while (!Ret_Block_queue.empty()) {
                LLVMBlock top_block = Ret_Block_queue.front();
                Ret_Block_queue.pop();
                auto Ret_ins = (RetInstruction *)top_block->Instruction_list.back();
                top_block->Instruction_list.pop_back();//压出ret指令
                top_block->InsertInstruction(1, new StoreInstruction(ret_type, ret_reg, Ret_ins->GetRetVal()));//为刚才的返回值寄存器赋值
                //跳转到统一块
                top_block->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(block->block_id)));
            }
            block->InsertInstruction(1, new LoadInstruction(ret_type, ret_reg, One_Ret_reg));//统一load到One_Reg
            block->InsertInstruction(1, new RetInstruction(ret_type, One_Ret_reg));
        } else {//函数返回值空，直接插跳转指令
            while (!Ret_Block_queue.empty()) {
                LLVMBlock top_block = Ret_Block_queue.front();
                Ret_Block_queue.pop();
                top_block->Instruction_list.pop_back();
                top_block->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(block->block_id)));
            }
            block->InsertInstruction(1, new RetInstruction(AllocaInstruction::VOID, nullptr));
        }
        C->ret_block = block;
        C->BuildCFG();
    }
    return ;
}

void DomAnalysis::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        MakeFunctionOneExit(cfg);
        DomInfo[cfg].C=cfg;
        DomInfo[cfg].BuildDominatorTree();
    }
}
void PostDomAnalysis::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        PostDomInfo[cfg].C=cfg;
        PostDomInfo[cfg].BuildDominatorTree(true);
    }
}
//计算后序遍历顺序
void dfs_ord(int cur, const std::vector<std::vector<LLVMBlock>> &G, std::vector<int> &result,
                   std::vector<int> &vsd) {
    vsd[cur] = 1;
    for (auto next_block : G[cur]) {
        if (vsd[next_block->block_id] == 0) {
            dfs_ord(next_block->block_id, G, result, vsd);
        }
    }
    result.push_back(cur);
}

//我们事实上没有显式构建支配树，而是经过思考后，隐式用bool二维数组dom计算出支配信息，成功完成了计算！
void DominatorTree::BuildDominatorTree(bool reverse) {
    dom_tree.clear();
    idom.clear();
    dom.clear();
    dom_ed.clear();

    //auto const *G=reverse?&(C->invG):&(C->G);
    //auto const *invG=reverse?&(C->G):&(C->invG);
    auto const *G = &(C->G);
    auto const *invG = &(C->invG);
    if (reverse) {
        auto temp = G;
        G = invG;
        invG = temp;
    }
    auto const begin_id=reverse?(C->ret_block->block_id):0;

    int block_num=C->top_label+1;

    
    dom_tree.resize(block_num);    
    idom.resize(block_num);    
    dom.resize(block_num, std::vector<bool>(block_num, false));
    dom_ed.resize(block_num);

    std::vector<int> ord_id;//后序遍历顺序
    std::vector<int> vsd(block_num, 0);//访问标记数组

    // std::vector<int> vsd;
    // for (int i = 0; i < block_num; i++) {
    //     vsd.push_back(0);
    //     //std::cout<<block_num<<" ";
    // }
    //求解后序遍历顺序
    dfs_ord(begin_id, (*G), ord_id, vsd);

    std::reverse(ord_id.begin(), ord_id.end());  // 转为逆后序（从根到叶子）

    for (int i = 0; i < block_num; i++) {
        if (i == begin_id) {
            dom[i][i] = true;  // 开始结点只支配自己
        } else {
           std::fill(dom[i].begin(), dom[i].end(), true);  // 初始化为支配所有结点
        }
    }

    // 迭代计算支配关系
    bool changed = true;
    while (changed) {
        changed = false;
        for (int u : ord_id) {
            if (u == begin_id) continue;

            std::vector<bool> new_dom(block_num, true); // 初始化为全 1
            bool has_predecessor = false;

            // 取所有前驱的交集
            for (auto pred_block : (*invG)[u]) {
                has_predecessor = true;
                for (int i = 0; i < block_num; i++) {
                    new_dom[i] = new_dom[i] && dom[pred_block->block_id][i];
                }
            }

            if (!has_predecessor) {//无前驱
                std::fill(new_dom.begin(), new_dom.end(), false);
                new_dom[u] = true;
            } else {
                new_dom[u] = true;
            }

            // 判断是否有变化
            if (new_dom != dom[u]) {
                dom[u] = new_dom;
                changed = true;
            }
            

        }
    }

    //构建被支配关系
    for (int u = 0; u < block_num; u++) {
        for (int v = 0; v < block_num; v++) {
            if (dom[u][v]) { // v 支配 u    && u != v 
                dom_ed[u].push_back(v);
            }
        }
    }

    //构建直接支配关系
    idom[begin_id] = (*C->block_map)[begin_id];
    for (int u = 0; u < block_num; u++) {
        if (u == begin_id||C->block_map->find(u)==C->block_map->end()) continue;
        int candidate = -1;
        for (int v : dom_ed[u]) {
            if (u != v) { // v 支配 u
                if (candidate == -1) {
                    candidate = v;
                } else if (dom_ed[candidate].end() == std::find(dom_ed[candidate].begin(), dom_ed[candidate].end(), v)) {
                candidate = v;
                }
            }
        }
         if(candidate != -1)idom[u] = (*C->block_map)[candidate];
    }

    // 构建支配树直接支配关系
    // for (int u = 0; u < block_num; u++) {
    //     if (u == begin_id) continue;
    //     int candidate = -1;
    //     for (int v = 0; v < block_num; v++) {
    //         if (dom[u][v] && u != v) { // v 支配 u
    //             if (candidate == -1) {
    //                 candidate = v;
    //             } else if (!dom[candidate][v]) { // 如果 candidate 不能支配 v
    //                 candidate = -1;
    //                 break;
    //             }
    //         }
    //     }
    //     if (candidate != -1) {
    //         idom[u] = (*C->block_map)[candidate];
    //         dom_tree[candidate].push_back((*C->block_map)[u]);
    //     }
    // }

    dom_frontier.clear();
    dom_frontier.resize(block_num);


    for (int n : ord_id) {
        //auto pres = C->GetPredecessor(n);
        auto pres = reverse ? C->GetSuccessor(n) : C->GetPredecessor(n);  
        if (pres.size() == 0) continue;
        int run = -1;
        for (auto pp : pres) {
            int p = pp->block_id;
            run = p;
            while (run != idom[n]->block_id) {
                dom_frontier[run].insert(n);
                run = idom[run]->block_id;
                if (run == -1) break;
            }
        }
    }

}


std::set<int> DominatorTree::GetDF(std::set<int> S) { 
    std::set<int> df_result;
    for (int id : S) {
        auto local_df = GetDF(id);
        df_result.insert(local_df.begin(), local_df.end());
    }
    return df_result; 
}

std::set<int> DominatorTree::GetDF(int id) { 
    // std::set<int> df_result;
    // for (auto succ : C->G[id]) {
    //     if (idom[succ->block_id] != (*C->block_map)[id]) {
    //         df_result.insert(succ->block_id);
    //     }
    // }
    // for (auto child : dom_tree[id]) {
    //     auto child_df = GetDF(child->block_id);
    //     for (int w : child_df) {
    //         if (idom[w] != (*C->block_map)[id]) {
    //             df_result.insert(w);
    //         }
    //     }
    // }
    // return df_result; 
    return dom_frontier[id];

}

bool DominatorTree::IsDominate(int id1, int id2) { return dom[id2][id1]; }