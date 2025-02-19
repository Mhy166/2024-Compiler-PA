#include "machine.h"
#include "assert.h"

//定义了几种常见的数据类型

MachineDataType INT32(MachineDataType::INT, MachineDataType::B32);
MachineDataType INT64(MachineDataType::INT, MachineDataType::B64);
MachineDataType INT128(MachineDataType::INT, MachineDataType::B128);

MachineDataType FLOAT_32(MachineDataType::FLOAT, MachineDataType::B32);
MachineDataType FLOAT64(MachineDataType::FLOAT, MachineDataType::B64);
MachineDataType FLOAT128(MachineDataType::FLOAT, MachineDataType::B128);

//此函数用于向 MachineCFG（控制流图）中添加一个新的节点：
//id：表示该节点（基本块）的唯一标识符
//Mblk：指向实际的基本块（MachineBlock）对象
void MachineCFG::AssignEmptyNode(int id, MachineBlock *Mblk) {
    if (id > this->max_label) {
        this->max_label = id;
    }
    MachineCFGNode *node = new MachineCFGNode;
    node->Mblock = Mblk;
    block_map[id] = node;
    while (G.size() < id + 1) {
        G.push_back({});
        // G.resize(id + 1);
    }
    while (invG.size() < id + 1) {
        invG.push_back({});
        // invG.resize(id + 1);
    }
}

//MakeEdge：用于在两个基本块之间建立一条边
//它接受两个基本块的 id，并在 G（正向邻接列表）和 invG（反向邻接列表）中添加相应的边。
// Just modify CFG edge, no change on branch instructions
void MachineCFG::MakeEdge(int edg_begin, int edg_end) {
    Assert(block_map.find(edg_begin) != block_map.end());
    Assert(block_map.find(edg_end) != block_map.end());
    Assert(block_map[edg_begin] != nullptr);
    Assert(block_map[edg_end] != nullptr);
    G[edg_begin].push_back(block_map[edg_end]);
    invG[edg_end].push_back(block_map[edg_begin]);
}

//RemoveEdge：用于删除 edg_begin 到 edg_end 之间的边
//它首先查找并删除 G 和 invG 中的相关条目。
// Just modify CFG edge, no change on branch instructions
void MachineCFG::RemoveEdge(int edg_begin, int edg_end) {
    assert(block_map.find(edg_begin) != block_map.end());
    assert(block_map.find(edg_end) != block_map.end());
    auto it = G[edg_begin].begin();
    for (; it != G[edg_begin].end(); ++it) {
        if ((*it)->Mblock->getLabelId() == edg_end) {
            break;
        }
    }
    G[edg_begin].erase(it);

    auto jt = invG[edg_end].begin();
    for (; jt != invG[edg_end].end(); ++jt) {
        if ((*jt)->Mblock->getLabelId() == edg_begin) {
            break;
        }
    }
    invG[edg_end].erase(jt);
}

//GetNewRegister：接受寄存器的类型和长度，创建一个新的虚拟寄存器，且寄存器编号递增。
Register MachineFunction::GetNewRegister(int regtype, int reglength) {
    static int new_regno = 0;
    Register new_reg;
    new_reg.is_virtual = true;
    new_reg.reg_no = new_regno++;
    new_reg.type.data_type = regtype;
    new_reg.type.data_length = reglength;
    
    return new_reg;
}

//GetNewReg：接受一个 MachineDataType 类型的数据类型，调用 GetNewRegister 创建寄存器。
Register MachineFunction::GetNewReg(MachineDataType type) { return GetNewRegister(type.data_type, type.data_length); }

//创建一个新的基本块
MachineBlock *MachineFunction::InitNewBlock() {
    int new_id = ++max_exist_label;
    MachineBlock *new_block = block_factory->CreateBlock(new_id);
    new_block->setParent(this);
    blocks.push_back(new_block);
    mcfg->AssignEmptyNode(new_id, new_block);
    return new_block;
}
