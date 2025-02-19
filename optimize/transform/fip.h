#ifndef FIP_H
#define FIP_H

#include "../pass.h"
#include <set>
#include <unordered_map>
#include <vector>
class CFG;
class LLVMIR;

class FIP : public IRPass {
private:
    void FI(LLVMIR *IR);

public:
    FIP(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

// class for FunctionCallGraph
class FunctionCallGraph {
public:
    CFG *MainCFG;        //reference！ 数据结构的设计有参考助教原框架！                                                                      
    std::unordered_map<CFG *, std::vector<CFG *>> CG;//控制流图                                          
    std::unordered_map<CFG *, std::unordered_map<CFG *, int>> CGNum;//调用数                          
    std::unordered_map<CFG *, std::unordered_map<CFG *, std::vector<Instruction>>> CGCallI;//对应调用数的调用指令   
    std::unordered_map<CFG *, size_t> CG_ins_num;//每个函数的指令数


    void BuildCG(LLVMIR *IR);
};


#endif