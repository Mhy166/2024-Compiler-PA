#include "physical_register.h"
bool PhysicalRegistersAllocTools::OccupyReg(int phy_id, LiveInterval interval) {
    // 你需要保证interval不与phy_id已有的冲突
    // 或者增加判断分配失败返回false的代码
    phy_occupied[phy_id].push_back(interval);
    return true;
}

bool PhysicalRegistersAllocTools::ReleaseReg(int phy_id, LiveInterval interval) {
    auto it = phy_occupied[phy_id].begin();
    while (it != phy_occupied[phy_id].end()) {
        if (*it == interval) {
            phy_occupied[phy_id].erase(it);
            return true;
        }
        it++;
    }
    return false;
}

bool PhysicalRegistersAllocTools::OccupyMem(int offset, LiveInterval interval,int size) {
    size /= 4;
    for (int i = offset; i < offset + size; i++) {
        while (i >= mem_occupied.size()) {
            mem_occupied.push_back({});
        }
        mem_occupied[i].push_back(interval);
    }
    return true;
}
bool PhysicalRegistersAllocTools::ReleaseMem(int offset,  LiveInterval interval,int size) {
    size /= 4;
    for (int i = offset; i < offset + size; i++) {
        auto iter = mem_occupied[i].begin();
        while(iter != mem_occupied[i].end()){
            if (*iter == interval) {
                mem_occupied[i].erase(iter);
                break;
            }
            iter++;
        }
    }
    return true;
}

int PhysicalRegistersAllocTools::getIdleReg(LiveInterval interval) {
    // 遍历所有有效寄存器
    std::map<int, int> reg_valid;
    for (auto i : getValidRegs(interval)) {
        reg_valid[i] = 1;
        bool flag = true;
        for (auto alias_reg : getAliasRegs(i)) {//所有别名
            for (auto interval_j : phy_occupied[alias_reg]) {
                if (interval & interval_j) { 
                    flag = false;
                    break;
                } 
            }
        }
        if (flag) {
            return i;  // 找到一个可用寄存器
        }
    }
    return -1;  // 如果没有找到合适的寄存器，返回 -1
}
int PhysicalRegistersAllocTools::getIdleMem(LiveInterval interval) {
    std::vector<bool> flags;
    for (int i = 0; i < mem_occupied.size(); ++i) {//都可以分配
        flags.push_back(true);
    }
    for (int i = 0; i < mem_occupied.size(); ++i) {
        for (auto interval_j : mem_occupied[i]) {
            if (interval & interval_j) {//冲突
                flags[i] = false;
                break;
            }
        }
    }
    int free_num = 0;//寻找连续的空闲区
    for (int offset = 0; offset < mem_occupied.size(); offset++) {
        if (flags[offset]) {
            free_num++;
        } else {
            free_num = 0;
        }
        if (free_num == interval.getReg().getDataWidth() / 4) {
            return offset - free_num + 1;
        }
    }
    return mem_occupied.size() - free_num;
}
//交换溢出和非溢出
int PhysicalRegistersAllocTools::swapRegspill(int p_reg1, LiveInterval interval1, int offset_spill2, int size,
                                              LiveInterval interval2) {
    ReleaseReg(p_reg1, interval1);
    ReleaseMem(offset_spill2,interval2,size);
    OccupyReg(p_reg1, interval2);
    return 0;
}

std::vector<LiveInterval> PhysicalRegistersAllocTools::getConflictIntervals(LiveInterval interval) {
    std::vector<LiveInterval> result;
    for (auto phy_intervals : phy_occupied) {//每个物理寄存器
        for (auto other_interval : phy_intervals) {//对于每个活跃区间，冲突了，且类型相同
            if (interval.getReg().type == other_interval.getReg().type && (interval & other_interval)) {
                result.push_back(other_interval);
            }
        }
    }
    return result;
}
