#ifndef CFG_ITERS_H
#define CFG_ITERS_H
#include "../machine.h"
#include <assert.h>
//Iterator 是一个抽象基类，用于定义各种类型的迭代器所需的方法
//每个子类都实现了具体的遍历方式，利用 Iterator 提供的接口访问 MachineCFG 中的基本块。

//顺序扫描控制流图中的基本块
class MachineCFG::SeqScanIterator : public Iterator {
private:
    decltype(block_map.begin()) current;

public:
    SeqScanIterator(MachineCFG *mcfg) : Iterator(mcfg) {}
    void open();//初始化迭代器
    MachineCFGNode *next();//返回下一个 MachineCFGNode
    bool hasNext();//检查是否还有下一个 MachineCFGNode
    void rewind();//将迭代器重置到初始位置
    void close();//关闭迭代器
};

//反向遍历
class MachineCFG::ReverseIterator : public Iterator {
private:
    Iterator *child;
    std::vector<MachineCFGNode *> cache;
    decltype(cache.rbegin()) current_pos;

public:
    ReverseIterator(Iterator *child) : Iterator(child->getMachineCFG()), child(child) {}
    void open();
    MachineCFGNode *next();
    bool hasNext();
    void rewind();
    void close();
};
//深度优先搜索（DFS）遍历
class MachineCFG::DFSIterator : public Iterator {
private:
    std::map<int, int> visited;
    std::stack<int> stk;

public:
    DFSIterator(MachineCFG *mcfg) : Iterator(mcfg) {}
    void open();
    MachineCFGNode *next();
    bool hasNext();
    void rewind();
    void close();
};
//广度优先搜索（BFS）遍历
class MachineCFG::BFSIterator : public Iterator {
private:
    std::map<int, int> visited;
    std::queue<int> que;

public:
    BFSIterator(MachineCFG *mcfg) : Iterator(mcfg) {}
    void open();
    MachineCFGNode *next();
    bool hasNext();
    void rewind();
    void close();
};

#endif