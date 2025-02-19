#include "cfg_iterators.h"
#include <assert.h>
//初始化广度优先搜索（BFS）迭代器，清空 visited 表示已访问的节点，并将起始节点（0）加入队列 que。
void MachineCFG::BFSIterator::open() {
    visited.clear();
    while (!que.empty())
        que.pop();
    que.push(0);
}
//返回队列中的第一个节点，并将该节点的所有未访问的邻接节点（即基本块）加入队列。然后从 block_map 中返回该节点。
MachineCFG::MachineCFGNode *MachineCFG::BFSIterator::next() {
    auto return_idx = que.front();
    visited[return_idx] = 1;
    que.pop();
    for (auto succ : mcfg->G[return_idx]) {
        if (visited[succ->Mblock->getLabelId()] == 0) {
            visited[succ->Mblock->getLabelId()] = 1;
            que.push(succ->Mblock->getLabelId());
        }
    }
    return mcfg->block_map[return_idx];
}
bool MachineCFG::BFSIterator::hasNext() { return !que.empty(); }
void MachineCFG::BFSIterator::rewind() { open(); }
void MachineCFG::BFSIterator::close() {
    visited.clear();
    while (!que.empty())
        que.pop();
}

//open()：初始化深度优先搜索（DFS）迭代器，清空 visited 和栈 stk，然后将起始节点（0）压入栈。
void MachineCFG::DFSIterator::open() {
    visited.clear();
    while (!stk.empty())
        stk.pop();
    stk.push(0);
}
//next()：返回栈顶的节点，并将其所有未访问的邻接节点压入栈中。然后返回该节点。
MachineCFG::MachineCFGNode *MachineCFG::DFSIterator::next() {
    auto return_idx = stk.top();
    visited[return_idx] = 1;
    stk.pop();
    for (auto succ : mcfg->G[return_idx]) {
        if (visited[succ->Mblock->getLabelId()] == 0) {
            visited[succ->Mblock->getLabelId()] = 1;
            stk.push(succ->Mblock->getLabelId());
        }
    }
    return mcfg->block_map[return_idx];
}
bool MachineCFG::DFSIterator::hasNext() { return !stk.empty(); }
void MachineCFG::DFSIterator::rewind() { open(); }
void MachineCFG::DFSIterator::close() {
    visited.clear();
    while (!stk.empty())
        stk.pop();
}
//open()：首先调用子迭代器（child）的 open() 方法，然后将子迭代器中的所有节点（基本块）存储到 cache 中
//最后，将 current_pos 设置为缓存的末尾，以便从反向开始遍历。
void MachineCFG::ReverseIterator::open() {
    child->open();
    cache.clear();
    while (child->hasNext()) {
        cache.push_back(child->next());
    }
    current_pos = cache.rbegin();
}
//next()：返回当前 current_pos 指向的基本块，并将指针向前移动。
MachineCFG::MachineCFGNode *MachineCFG::ReverseIterator::next() { return *(current_pos++); }
bool MachineCFG::ReverseIterator::hasNext() { return current_pos != cache.rend(); }
void MachineCFG::ReverseIterator::rewind() {
    close();
    open();
}
void MachineCFG::ReverseIterator::close() {
    child->close();
    cache.clear();
    current_pos = cache.rend();
}

//open()：初始化顺序扫描迭代器，设置 current 为 block_map 的第一个元素。
void MachineCFG::SeqScanIterator::open() { current = mcfg->block_map.begin(); }
//next()：返回当前 current 指向的基本块，并将 current 指向下一个元素。
MachineCFG::MachineCFGNode *MachineCFG::SeqScanIterator::next() { return (current++)->second; }
bool MachineCFG::SeqScanIterator::hasNext() { return current != mcfg->block_map.end(); }
void MachineCFG::SeqScanIterator::rewind() {
    close();
    open();
}
void MachineCFG::SeqScanIterator::close() { current = mcfg->block_map.end(); }

//创建不同类型的迭代器实例
MachineCFG::DFSIterator *MachineCFG::getDFSIterator() { return new DFSIterator(this); }
MachineCFG::BFSIterator *MachineCFG::getBFSIterator() { return new BFSIterator(this); }
MachineCFG::SeqScanIterator *MachineCFG::getSeqScanIterator() { return new SeqScanIterator(this); }
MachineCFG::ReverseIterator *MachineCFG::getReverseIterator(Iterator *child) { return new ReverseIterator(child); }
