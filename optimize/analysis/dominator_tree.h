#ifndef DOMINATOR_TREE_H
#define DOMINATOR_TREE_H
#include "../../include/ir.h"
#include "../pass.h"
#include <set>
#include <vector>
//支配树
class DominatorTree {
public:
    CFG *C;
    std::vector<std::vector<LLVMBlock>> dom_tree{};
    std::vector<LLVMBlock> idom{};
    void BuildDominatorTree(bool reverse = false);    // 为一个CFG构建支配树
    std::set<int> GetDF(std::set<int> S);             // 返回节点集合的支配边界，会用下面的函数
    std::set<int> GetDF(int id);                      // 返回单个节点的支配边界
    bool IsDominate(int id1, int id2);                // 是否有支配关系

    //(失败的尝试)
    //假设 id_to_index_map 用于将 block_id 映射到索引
    //void dfs_ord(int cur, const std::vector<std::vector<LLVMBlock>> &G, std::vector<int> &result,
    //               std::vector<int> &vsd) ;
    //std::map<int, int> id_to_index_map;
    //std::vector<int> valid_block_ids;  // 存储有效的 block_id 列表

    //自行添加数据结构
    std::vector<std::set<int>> dom_frontier{};        // 支配边界 dom_frontier[i]: i 的支配边界有哪些
    std::vector<std::vector<bool>> dom{};             // 是否支配
    std::vector<std::vector<int>> dom_ed{};           // 被支配
};
//正向支配分析
class DomAnalysis : public IRPass {
    private:
    std::map<CFG *, DominatorTree> DomInfo;

    public:
    DomAnalysis(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
    DominatorTree *GetDomTree(CFG *C) { return &DomInfo[C]; }
};
//逆向支配分析
class PostDomAnalysis : public IRPass {
    private:
    std::map<CFG *, DominatorTree> PostDomInfo;

    public:
    PostDomAnalysis(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
    DominatorTree *GetPostDomTree(CFG *C) { return &PostDomInfo[C]; }
};
#endif