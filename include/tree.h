#ifndef TREE_H
#define TREE_H

// tree definition (used for AST)
#include "type.h"
#include <iostream>

class tree_node {
protected:
    int line_number;
    //对于语法树，不仅有行号，而且要有节点属性类型-->Type和ConstValue
public:
    NodeAttribute attribute;    // 在类型检查阶段需要阅读该类的代码，语法分析阶段可以先忽略该变量
    VarAttribute var;
    int GetLineNumber() { return line_number; }
    void SetLineNumber(int t) { line_number = t; }

    NodeAttribute GetNodeAttribute(){return attribute;}
    void SetNodeAttribute(NodeAttribute node){attribute=node;}
    


    virtual void codeIR() = 0;                              // 中间代码生成
    virtual void printAST(std::ostream &s, int pad) = 0;    // 打印语法树
    virtual void TypeCheck() = 0;                           // 类型检查
};

#endif