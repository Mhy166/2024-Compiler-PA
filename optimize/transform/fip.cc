#include "fip.h"
#include <algorithm>
//CG构建
FunctionCallGraph fcallgraph;

//调用BuildCG()和BuildFunctionInfo()
void LLVMIR::BuildCG(){
    fcallgraph.BuildCG(this);
}
void FunctionCallGraph::BuildCG(LLVMIR *IR) {
    CG_ins_num.clear();
    CGCallI.clear();
    CGNum.clear();
    CG.clear();
    MainCFG = IR->CFGMap["main"];
    for (auto [FuncDefI, CFG] : IR->llvm_cfg) {
        auto FuncName = FuncDefI->GetFunctionName();
        if (IR->CFGMap.find(FuncName) == IR->CFGMap.end()) {
            continue;
        }
        CG_ins_num[CFG] = 0;
        //每个块
        for (auto [id, bb] : *CFG->block_map) {
            //每个指令
            for (auto it = --bb->Instruction_list.end();; --it) {
                auto ins = *it;
                CG_ins_num[CFG]++;//指令+1
                if (ins->GetOpcode() != BasicInstruction::LLVMIROpcode::CALL) {
                    if (it == bb->Instruction_list.begin()) {
                        break;
                    }
                    continue;
                }
                //一条Call
                auto CallI = (CallInstruction *)ins;
                auto vFuncName = CallI->GetFunctionName();
                //寻找调用的函数的CFG
                if (IR->CFGMap.find(vFuncName) == IR->CFGMap.end()) {
                    if (it == bb->Instruction_list.begin()) {
                        break;
                    }
                    continue;
                }
                auto callee_CFG = IR->CFGMap[vFuncName];
                CGCallI[CFG][callee_CFG].push_back(ins);//CFG调用callee_cfg
                if (CGNum.find(CFG) == CGNum.end()||CGNum[CFG].find(callee_CFG) == CGNum[CFG].end()) {
                    //还未曾调用过任何函数或者是它
                    CGNum[CFG][callee_CFG] = 1;
                    CG[CFG].push_back(callee_CFG);
                } else {
                    CGNum[CFG][callee_CFG]++;
                }
                if (it == bb->Instruction_list.begin()) {
                    break;
                }
            }
        }
    }
}
//内联
static std::unordered_map<CFG *, bool> CFG_flag;
static std::unordered_map<Instruction, bool> reinlinecall_ins;

void FIP::Execute() {
    FI(llvmIR);
}

Operand InlineCFG(CFG *uCFG, CFG *vCFG, LLVMBlock StartBB, LLVMBlock EndBB, std::map<int, int> &reg_replace_map,
                  std::map<int, int> &label_replace_map) {
    //0号块映射StartBB
    label_replace_map[0] = StartBB->block_id;
    for (auto [id, bb] : *vCFG->block_map) {
        if (id == 0 || bb->Instruction_list.empty()) {
            continue;
        }
        auto newbb = uCFG->NewBlock();
        label_replace_map[bb->block_id] = newbb->block_id;
    }
    //被内联的函数所有块都映射好了，也就是label

    for (auto [id, bb] : *vCFG->block_map) {
        auto nowbb_id = label_replace_map[id];//新label
        auto nowbb = (*uCFG->block_map)[nowbb_id];
        for (auto ins : bb->Instruction_list) {//遍历每条指令
            //返回指令，变成无条件跳转到EndBlock
            if (ins->GetOpcode() == BasicInstruction::LLVMIROpcode::RET) {
                auto new_ins = new BrUncondInstruction(GetNewLabelOperand(EndBB->block_id));
                nowbb->InsertInstruction(1, new_ins);//插到末尾
                continue;
            }
            Instruction now_ins;
            now_ins = ins->CopyInstruction();//复制一份
            now_ins->Label_Rename(label_replace_map);
            auto use_ops = now_ins->GetNonResultOperands();
            for (auto &op : use_ops) {
                if (op->GetOperandType() != BasicOperand::REG) {
                    continue;
                }
                auto RegOp = (RegOperand *)op;
                auto RegNo = RegOp->GetRegNo();
                //被内联指令的寄存器号
                if (reg_replace_map.find(RegNo) == reg_replace_map.end()) {
                    auto newNo = ++uCFG->top_reg;
                    op = GetNewRegOperand(newNo);//新的寄存器
                    reg_replace_map[RegNo] = newNo;
                }
            }
            auto ResultReg = (RegOperand *)now_ins->GetResultReg();

            if (ResultReg != NULL && reg_replace_map.find(ResultReg->GetRegNo()) == reg_replace_map.end()) {
                auto ResultRegNo = ResultReg->GetRegNo();
                int newNo = ++uCFG->top_reg;
                auto newReg = GetNewRegOperand(newNo);
                reg_replace_map[ResultRegNo] = newNo;
            }
            //新指令需要重命名，到本地的寄存器号
            now_ins->Reg_Rename(reg_replace_map);
            nowbb->InsertInstruction(1, now_ins);
            now_ins->BlockID=nowbb_id;
        }
    }
    //拷贝最终返回指令
    auto retBB = (BasicBlock *)vCFG->ret_block;
    auto RetI = (RetInstruction *)retBB->Instruction_list.back();
    auto RetResult = RetI->GetRetVal();

    if (RetResult != NULL) {
        if (RetResult->GetOperandType() == BasicOperand::REG) {
            auto oldRegNo = ((RegOperand *)RetResult)->GetRegNo();
            return GetNewRegOperand(reg_replace_map[oldRegNo]);
        }
        return RetResult;
    }
    return nullptr;
}
//reference！ 内联的大体逻辑，挂接块等结构部分最初很懵，有参考助教原框架代码！
void InlineCFG(CFG *uCFG, CFG *vCFG, uint32_t CallI_No,LLVMIR *IR) {
    std::map<int, int> reg_replace_map;
    std::map<int, int> label_replace_map;
    std::set<Instruction> EraseSet;
    auto StartBB = uCFG->NewBlock();
    auto EndBB = uCFG->NewBlock();
    auto StartBB_id = StartBB->block_id;
    
    auto EndBB_id = EndBB->block_id;

    int formal_regno = -1;
    auto CallI = (CallInstruction *)fcallgraph.CGCallI[uCFG][vCFG][CallI_No];//找到那条函数调用指令
    auto BlockMap = *(uCFG->block_map);
    //调用指令所处的块
    auto uCFG_BBid = CallI->BlockID;
    auto oldbb = BlockMap[CallI->BlockID];
    //对调用指令的实参进行处理！
    for (auto formal : CallI->GetParameterList()) {
        ++formal_regno;
        if (formal.second->GetOperandType() == BasicOperand::IMMI32) {
            //需要包装成寄存器，以便后续处理,需要把PHI都移出，放进指令，再移回来
            std::stack<Instruction> Phi_Transfer;//存储所有PHI指令
            while (oldbb->Instruction_list.front()->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                Phi_Transfer.push(oldbb->Instruction_list.front());
                oldbb->Instruction_list.pop_front();
            }
            auto newAddReg = GetNewRegOperand(++uCFG->top_reg);
            oldbb->InsertInstruction(0, new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::LLVMType::I32, formal.second, new ImmI32Operand(0), newAddReg));
            while (!Phi_Transfer.empty()) {
                oldbb->InsertInstruction(0, Phi_Transfer.top());
                Phi_Transfer.pop();
            }
            reg_replace_map[formal_regno] = newAddReg->GetRegNo();
            fcallgraph.CG_ins_num[uCFG]++;//指令多了一个包装Reg指令
        } else if (formal.second->GetOperandType() == BasicOperand::IMMF32) {
            std::stack<Instruction> Phi_Transfer;
            while (oldbb->Instruction_list.front()->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                Phi_Transfer.push(oldbb->Instruction_list.front());
                oldbb->Instruction_list.pop_front();
            }
            auto newAddReg = GetNewRegOperand(++uCFG->top_reg);
            oldbb->InsertInstruction(0, new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::FADD, BasicInstruction::LLVMType::FLOAT32, formal.second, new ImmF32Operand(0), newAddReg));
            while (!Phi_Transfer.empty()) {
                oldbb->InsertInstruction(0, Phi_Transfer.top());
                Phi_Transfer.pop();
            }
            reg_replace_map[formal_regno] = newAddReg->GetRegNo();
            fcallgraph.CG_ins_num[uCFG]++;
        } else {//形参以寄存器形式，直接加入置换表
            reg_replace_map[formal_regno] = ((RegOperand *)formal.second)->GetRegNo();
        }
    }

    Operand NewResultOperand = InlineCFG(uCFG, vCFG, StartBB, EndBB, reg_replace_map, label_replace_map);
    //旧的0号分配块被分配到新的Start块里，需要把其中的ALLOCA都移到父亲函数的0号块。
    auto vfirstlabelno = label_replace_map[0];
    auto uAllocaBB = (BasicBlock *)BlockMap[vfirstlabelno];
    for (auto I : uAllocaBB->Instruction_list) {
        if (I->GetOpcode() != BasicInstruction::LLVMIROpcode::ALLOCA) {
            continue;
        }
        BlockMap[0]->InsertInstruction(0, I);
        EraseSet.insert(I);
    }
    //清除分配块中的ALLOCA
    auto tmp_list = uAllocaBB->Instruction_list;
    uAllocaBB->Instruction_list.clear();
    for (auto ins : tmp_list) {
        if (EraseSet.find(ins) != EraseSet.end()) {
            continue;
        }
        uAllocaBB->InsertInstruction(1, ins);
    }
    EraseSet.clear();
    //刚才内联的结果寄存器->正常调用的结果寄存器，使得在原函数里正确衔接后面的使用
    if (CallI->GetRetType() != BasicInstruction::LLVMType::VOID) {
        if (CallI->GetResultType() == BasicInstruction::LLVMType::I32) {
            EndBB->InsertInstruction(1, new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::LLVMType::I32, (ImmI32Operand *)NewResultOperand,
                                                                  new ImmI32Operand(0), CallI->GetResultReg()));
        } else {
            EndBB->InsertInstruction(1, new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::FADD, BasicInstruction::LLVMType::FLOAT32, (ImmF32Operand *)NewResultOperand,
                                                                  new ImmF32Operand(0), CallI->GetResultReg()));
        }
        fcallgraph.CG_ins_num[uCFG]++;
    }

    auto StartBB_label = GetNewLabelOperand(StartBB_id);
    BrUncondInstruction *newBrI;
    auto oldbb_it = oldbb->Instruction_list.begin();
    bool EndbbBegin = false;
    for (auto it = oldbb->Instruction_list.begin(); it != oldbb->Instruction_list.end(); ++it) {
        auto I = *it;//遍历原来调用的每条指令
        if (!EndbbBegin && I == CallI) {//定位调用指令
            EndbbBegin = true;
            newBrI = new BrUncondInstruction(StartBB_label);//建一个无条件跳转指令
            newBrI->BlockID=oldbb->block_id;
            oldbb_it = it;
        } else if (EndbbBegin) {
            I->BlockID=EndBB_id;//后面的指令都改换门庭，插到End里面
            EndBB->InsertInstruction(1, I);
        }
    }

    while (oldbb_it != oldbb->Instruction_list.end()) {
        oldbb->Instruction_list.pop_back();
    }
    //使得老块跳到Start块里
    oldbb->InsertInstruction(1, newBrI);
    label_replace_map.clear();
    label_replace_map[oldbb->block_id] = EndBB->block_id;
    for (auto nextBB : uCFG->G[uCFG_BBid]) {//它的后继块如果有PHI指令也需要改换门庭
        for (auto I : nextBB->Instruction_list) {
            if (I->GetOpcode() != BasicInstruction::LLVMIROpcode::PHI) {
                continue;
            }
            auto PhiI = (PhiInstruction *)I;
            PhiI->Label_Rename(label_replace_map);
        }
    }
    label_replace_map.clear();
}
void InlineDFS(CFG *caller_CFG,LLVMIR *IR) {
    if (CFG_flag.find(caller_CFG) != CFG_flag.end()) {//遍历过了，不再遍历
        return;
    }
    CFG_flag[caller_CFG] = true;

    for (auto callee_CFG : fcallgraph.CG[caller_CFG]) {
        if (callee_CFG == caller_CFG) {
            continue;
        }
        InlineDFS(callee_CFG,IR);
        if (fcallgraph.CG.find(callee_CFG) == fcallgraph.CG.end()||
            fcallgraph.CGNum[callee_CFG].find(callee_CFG) != fcallgraph.CGNum[callee_CFG].end()) {//调用函数是递归函数，那没办法展开
            continue;
        }
        int map_size = fcallgraph.CGNum[caller_CFG][callee_CFG];
        for (uint32_t i = 0; i < map_size; ++i) {
            if (fcallgraph.CG_ins_num[callee_CFG] >= 100) {//阈值设置100
                break;
            }
            InlineCFG(caller_CFG, callee_CFG, i, IR);//执行内联
            fcallgraph.CG_ins_num[caller_CFG] += fcallgraph.CG_ins_num[callee_CFG];//指令数加上去
            caller_CFG->BuildCFG();//内联上后，需要重新构建CFG
        }
    }
}

void FIP::FI(LLVMIR *IR){
    CFG_flag.clear();
    reinlinecall_ins.clear();
    InlineDFS(fcallgraph.MainCFG,IR);
}
