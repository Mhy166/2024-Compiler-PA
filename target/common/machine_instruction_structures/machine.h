#ifndef MACHINE
#define MACHINE

#include "../../../include/ir.h"
#include "../MachineBaseInstruction.h"
class MachineFunction;
class MachineBlock;
class MachineUnit;
class MachineCFG;
#include <list>

//MachineBlock 类表示一个基本块，包含了一系列的机器指令。每个块有一个唯一的 label_id
class MachineBlock {
    private:
    int label_id;

    protected:
    std::list<MachineBaseInstruction *> instructions;

    private:
    MachineFunction *parent;

    public:
    decltype(instructions) &GetInsList() { return instructions; }
    void clear() { instructions.clear(); }
    auto erase(decltype(instructions.begin()) it) { return instructions.erase(it); }
    auto insert(decltype(instructions.begin()) it, MachineBaseInstruction *ins) { return instructions.insert(it, ins); }
    auto getParent() { return parent; }
    void setParent(MachineFunction *parent) { this->parent = parent; }
    auto getLabelId() { return label_id; }
    auto ReverseBegin() { return instructions.rbegin(); }
    auto ReverseEnd() { return instructions.rend(); }
    auto begin() { return instructions.begin(); }
    auto end() { return instructions.end(); }
    auto size() { return instructions.size(); }
    void push_back(MachineBaseInstruction *ins) { instructions.push_back(ins); }
    void push_front(MachineBaseInstruction *ins) { instructions.push_front(ins); }
    void pop_back() { instructions.pop_back(); }
    void pop_front() { instructions.pop_front(); }
    //第一个指令的编号-1
    int getBlockInNumber() {
        for (auto ins : instructions) {
            return ins->getNumber() - 1;
        }
        ERROR("Empty block");
        // return (*(instructions.begin()))->getNumber();
    }
    //最后一个指令的编号
    int getBlockOutNumber() {
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
            auto ins = *it;
            return ins->getNumber();
        }
        ERROR("Empty block");
        // return (*(instructions.rbegin()))->getNumber();
    }
    MachineBlock(int id) : label_id(id) {}
};

//用于创建 MachineBlock
class MachineBlockFactory {
public:
    virtual MachineBlock *CreateBlock(int id) = 0;
};

//MachineFunction 表示一个机器级的函数，包含函数的名称、形参、栈大小等
class MachineFunction {
    // 你可以根据需要自行修改，新增，删除已有的成员函数和成员变量
    private:
    // 函数名
    std::string func_name;
    // 所属的MachineUnit
    MachineUnit *parent;
    // 函数形参列表
    std::vector<Register> parameters;
    // 现存的最大块编号
    int max_exist_label;

    // 用于基本块相关操作的辅助类
    MachineBlockFactory *block_factory;

    protected:
    // 栈大小
    int stack_sz;
    // 传实参所用到的栈空间大小
    int para_sz;
    MachineCFG *mcfg;

    public:
    // 更新现存的最大块编号
    void UpdateMaxLabel(int labelid) { max_exist_label = max_exist_label > labelid ? max_exist_label : labelid; }
    // 获取形参列表
    const decltype(parameters) &GetParameters() { return parameters; }
    // 增加形参列表
    void AddParameter(Register reg) { parameters.push_back(reg); }
    // 设置栈大小
    void SetStackSize(int sz) { stack_sz = sz; }
    // 更新传实参所用到的栈空间大小
    void UpdateParaSize(int parasz) {
        if (parasz > para_sz) {
            para_sz = parasz;
        }
    }
    // 获取传实参所用到的栈空间大小
    int GetParaSize() { return para_sz; }
    // 增加栈大小
    virtual void AddStackSize(int sz) { stack_sz += sz; }
    // 获取栈空间大小（自动对齐）
    int GetStackSize() { return ((stack_sz + 15) / 16) * 16; }
    // 获取栈底偏移
    int GetStackOffset() { return stack_sz; }
    // 获取CFG
    MachineCFG *getMachineCFG() { return mcfg; }
    // 获取MachineUnit
    MachineUnit *getParentMachineUnit() { return parent; }
    // 获取函数名
    std::string getFunctionName() { return func_name; }

    // 设置MachineUnit
    void SetParent(MachineUnit *parent) { this->parent = parent; }
    // 设置CFG
    void SetMachineCFG(MachineCFG *mcfg) { this->mcfg = mcfg; }

    // 函数包含的所有块
    std::vector<MachineBlock *> blocks{};

    public:
    // 获取新的虚拟寄存器
    Register GetNewRegister(int regtype, int regwidth);
    // 获取新的虚拟寄存器，等同于GetNewRegister(type.data_type, type.data_length)
    Register GetNewReg(MachineDataType type);

    protected:
    // 获取新的块
    MachineBlock *InitNewBlock();

    public:
    MachineFunction(std::string name, MachineBlockFactory *blockfactory)
        : func_name(name), stack_sz(0), para_sz(0), block_factory(blockfactory), max_exist_label(0){}
};

//MachineUnit 是整个机器单元的集合，包含多个 MachineFunction
class MachineUnit {
    public:
    // 指令选择时，对全局变量不作任何处理，直接保留到MachineUnit中
    std::vector<Instruction> global_def{};

    std::vector<MachineFunction *> functions;
};

//MachineCFG 表示控制流图
//它维护了一个有向图，表示程序的控制流关系
//每个图节点代表一个 MachineBlock，而图的边代表不同基本块之间的控制流关系
class MachineCFG {
    public:
    class MachineCFGNode {
    public:
        MachineBlock *Mblock;
    };

    private:
    std::map<int, MachineCFGNode *> block_map{};//很熟悉
    std::vector<std::vector<MachineCFGNode *>> G{}, invG{};
    int max_label;

    public:
    MachineCFG() : max_label(0){};
    void AssignEmptyNode(int id, MachineBlock *Mblk);

    // Just modify CFG edge, no change on branch instructions
    void MakeEdge(int edg_begin, int edg_end);
    void RemoveEdge(int edg_begin, int edg_end);

    MachineCFGNode *GetNodeByBlockId(int id) { return block_map[id]; }
    std::vector<MachineCFGNode *> GetSuccessorsByBlockId(int id) { return G[id]; }
    std::vector<MachineCFGNode *> GetPredecessorsByBlockId(int id) { return invG[id]; }

    private:

    //Iterator 模式用于遍历控制流图。它定义了一个基础接口和多个实现：

    //SeqScanIterator：顺序扫描迭代器。
    //ReverseIterator：逆向迭代器。
    //DFSIterator：深度优先迭代器。
    //BFSIterator：广度优先迭代器。
    class Iterator {
    protected:
        MachineCFG *mcfg;

    public:
        Iterator(MachineCFG *mcfg) : mcfg(mcfg) {}
        MachineCFG *getMachineCFG() { return mcfg; }
        virtual void open() = 0;
        virtual MachineCFGNode *next() = 0;
        virtual bool hasNext() = 0;
        virtual void rewind() = 0;
        virtual void close() = 0;
    };
    class SeqScanIterator;
    class ReverseIterator;
    class DFSIterator;
    class BFSIterator;

    public:
    DFSIterator *getDFSIterator();
    BFSIterator *getBFSIterator();
    SeqScanIterator *getSeqScanIterator();
    ReverseIterator *getReverseIterator(Iterator *);
};

#include "cfg_iterators/cfg_iterators.h"
#endif